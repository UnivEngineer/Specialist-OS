/* On-machine container conversion. Input is never opened for writing. */
#include "mxos.h"
#include "mxos_runtime.h"
#include <string.h>

static MXOS_FILE input,output,existing;
static MXOS_IMAGE_HEADER image;
static unsigned char buffer[1024],prefix[32];
static char command[130];
static char *source,*target;
static unsigned int size,skip,expected,body_sum;
static unsigned char check_input,overwrite,raw_input;
static unsigned char have_load,have_entry,have_begin,have_end,have_policy;
static unsigned int new_load,new_entry,new_begin,new_end;
static unsigned char new_policy;

static int same(const char *a,const char *b) {
 while(*a && *b && mxos_upper(*a)==mxos_upper(*b)){++a;++b;}
 return !*a && !*b;
}
static unsigned int word(const unsigned char *p){return p[0]|((unsigned int)p[1]<<8);}
static void putword(unsigned char *p,unsigned int n){p[0]=n;p[1]=n>>8;}
static int hex(const char *p,unsigned int *n) {
 unsigned int value=0;unsigned char digits=0,c;
 if(!*p)return 0;
 while(*p){c=mxos_upper(*p++);if(c>='0'&&c<='9')c-='0';else if(c>='A'&&c<='F')c-='A'-10;else return 0;if(++digits>4)return 0;value=(value<<4)|c;}
 *n=value;return 1;
}
static unsigned int sum(const unsigned char *p,unsigned int n) {
 unsigned char lo=0,hi=0;unsigned int i,v;
 for(i=0;i<n;++i){v=(unsigned int)lo+p[i];lo=v;if(i+1<n)hi+=p[i]+(v>>8);}
 return ((unsigned int)hi<<8)|lo;
}
static int fail(const char *text){mxos_puts(text);mxos_putch(10);return 1;}
static int flags(const char *p){if(same(p,"MAIN"))return 4;if(same(p,"BANK"))return 2;if(same(p,"DOS"))return 1;if(same(p,"LEGACY"))return 0;return -1;}
static int policy(const char *p){if(same(p,"EXCLUSIVE"))return 0;if(same(p,"PRESERVE"))return 1;if(same(p,"MANAGED"))return 2;return -1;}
static int mode(const char *p){if(same(p,"AUTO"))return 0;if(same(p,"MX"))return 1;if(same(p,"STD"))return 2;if(same(p,"MON"))return 3;if(same(p,"RF"))return 4;return -1;}
static int option(char *p) {
 char *v;unsigned int n;int f;
 if(same(p,"Y")){overwrite=1;return 1;}
 if(same(p,"RAW")){raw_input=1;return 1;}
 v=strchr(p,'=');if(!v)v=strchr(p,':');if(!v)return 0;*v++=0;
 if(same(p,"F")){f=flags(v);if(f<0)return 0;image.flags=f;image.api=f?2:0;return 1;}
 if(same(p,"P")){f=policy(v);if(f<0)return 0;new_policy=f;have_policy=1;return 1;}
 if(same(p,"MODE")){f=mode(v);if(f<0)return 0;image.mode=f;return 1;}
 if(!hex(v,&n))return 0;
 if(same(p,"L")){new_load=n;have_load=1;}
 else if(same(p,"E")){new_entry=n;have_entry=1;}
 else if(same(p,"BEGIN")){new_begin=n;have_begin=1;}
 else if(same(p,"END")){new_end=n;have_end=1;}
 else if(same(p,"B"))image.bss=n;
 else if(same(p,"HEAP"))image.min_heap=n;
 else if(same(p,"STACK"))image.stack_bytes=n;
 else if(same(p,"CACHE")&&n<2)image.options=n;
 else return 0;
 return 1;
}
static int scan_sum(void) {
 unsigned int left=size,n,i,v;unsigned char lo=0,hi=0;
 if(mxos_fseek(&input,skip,MXOS_SEEK_SET))return 0;
 while(left){n=left>sizeof(buffer)?sizeof(buffer):left;if(mxos_read(&input,buffer,n)!=n)return 0;
  for(i=0;i<n;++i){v=(unsigned int)lo+buffer[i];lo=v;if(left>1)hi+=buffer[i]+(v>>8);--left;}
 }
 body_sum=((unsigned int)hi<<8)|lo;
 return !check_input || body_sum==expected;
}
int main(void) {
 char *p,*start,*ext;unsigned int n,file_size,left,block,end;int f,is_rks=0;
 if(sizeof(image)!=32)return fail("HEADER LAYOUT ERROR");
 mxos_puts("EXE 1.0\n");
 /* Tokenize a private copy: source/target cannot alias the original argument line. */
 n=0;p=mxos_args;while(*p&&n<128)command[n++]=*p++;command[n]=0;
 p=command;
 while(*p){while(*p==' ')++p;if(!*p)break;start=p;while(*p&&*p!=' ')++p;if(*p)*p++=0;
  if(*start=='-')continue;
  if(!source)source=start;else if(!target)target=start;else return fail("INVALID ARGUMENTS");
 }
 if(!source||!target){mxos_puts("EXE source target [-F=MAIN|BANK|DOS|LEGACY] [-Y]\n-B=hex -L=hex -E=hex -BEGIN=hex -END=hex\n-P=EXCLUSIVE|PRESERVE|MANAGED -HEAP=hex -STACK=hex\n-MODE=AUTO|MX|STD|MON|RF -CACHE=0|1 -RAW\nAll numbers are hexadecimal. Source is preserved.\n");return 1;}
 if(same(source,target))return fail("SOURCE AND TARGET MUST DIFFER");
 /* First pass for -RAW before detecting the input container. */
 p=command;while(p<command+n){if(*p=='-'&&same(p+1,"RAW"))raw_input=1;p+=strlen(p)+1;}
 if(mxos_fopen(&input,source,"rb"))return fail("CANNOT OPEN SOURCE");
 if(input.bytes[30]||input.bytes[31]){mxos_fclose(&input);return fail("SOURCE IS TOO LARGE");}
 file_size=word(input.bytes+28);size=file_size;
 image.bss=0xffff;image.policy=0;
 ext=strrchr(source,'.');
 if(!raw_input&&file_size>=16&&mxos_read(&input,prefix,32)>=16&&prefix[0]==0xcd&&prefix[1]==0x20&&prefix[2]=='M'&&prefix[3]=='X'){
  skip=prefix[4]==1?16:32;
  if((prefix[4]!=1&&prefix[4]!=2)||file_size<skip||sum(prefix,skip-2)!=word(prefix+skip-2))goto bad;
  image.load=word(prefix+6);image.entry=word(prefix+8);size=word(prefix+10);expected=word(prefix+12);check_input=1;
  if(file_size-skip!=size)goto bad;
  if(skip==32){memcpy(&image,prefix,32);if(image.header_bytes!=32||image.reserved)goto bad;}
  else{image.flags=prefix[5];image.api=image.flags?2:0;if(image.flags){image.bss=0;image.policy=2;}}
 }else if(!raw_input&&ext&&same(ext,".RKS")){
  if(file_size<7||mxos_fseek(&input,0,MXOS_SEEK_SET)||mxos_read(&input,prefix,4)!=4)goto bad;
  image.load=word(prefix);image.entry=image.load;end=word(prefix+2);if(end<image.load)goto bad;
  size=end-image.load+1;skip=4;if(file_size-6!=size)goto bad;
  if(mxos_fseek(&input,-2,MXOS_SEEK_END)||mxos_read(&input,prefix,2)!=2)goto bad;
  expected=word(prefix);check_input=1;
 }
 if(!size||!scan_sum())goto bad;
 /* Apply options only after importing the original metadata. */
 p=command;while(p<command+n){if(*p=='-'&&!option(p+1))goto arguments;p+=strlen(p)+1;}
 if(have_load){image.load=new_load;if(!have_entry)image.entry=new_load;}
 if(have_entry)image.entry=new_entry;
 if(have_policy)image.policy=new_policy;else if(have_load||skip!=32)image.policy=image.flags?2:0;
 if(!image.flags)image.api=0;
 if(have_begin)image.mem_begin=new_begin;else if(skip!=32||have_load)image.mem_begin=image.load;
 if(image.load+size<image.load)goto arguments;
 end=image.load+size;
 if(image.entry<image.load||image.entry>=end)goto arguments;
 if(image.bss!=0xffff){if(end+image.bss<end)goto arguments;end+=image.bss;}
 if(have_end)image.mem_end=new_end;else if(skip!=32||have_load||image.mem_end<end)image.mem_end=end;
 if(image.mem_begin>image.load||image.mem_end<end||image.mem_end+image.min_heap<image.mem_end)goto arguments;
 if((image.bss==0xffff&&image.policy!=0)||image.policy>2||image.mode>4||image.api!=0&&image.api!=2||image.options>1||image.flags!=0&&image.flags!=1&&image.flags!=2&&image.flags!=4)goto arguments;
 ext=strrchr(target,'.');if(!ext)goto arguments;
 if(same(ext,".RKS")){if(image.entry!=image.load)goto arguments;is_rks=1;}
 else if(!same(ext,".EXE")&&!same(ext,".COM")&&!same(ext,".DLL")&&!same(ext,".FNT"))goto arguments;
 /* Detect identical files through relative/absolute path aliases before truncation. */
 if(!mxos_fopen(&existing,target,"rb")){
  f=existing.bytes[40]==input.bytes[40]&&word(existing.bytes+26)==word(input.bytes+26);mxos_fclose(&existing);
  if(f){mxos_fclose(&input);return fail("SOURCE AND TARGET MUST DIFFER");}
  if(!overwrite){mxos_puts("OVERWRITE FILE? [Y/N] ");f=mxos_upper(mxos_getch());mxos_putch(10);if(f!='Y'){mxos_fclose(&input);return 1;}}
 }
 if(mxos_fseek(&input,skip,MXOS_SEEK_SET)||mxos_fopen(&output,target,"wb"))goto bad;
 if(is_rks){putword(prefix,image.load);putword(prefix+2,image.load+size-1);block=4;}
 else{image.signature[0]=0xcd;image.signature[1]=0x20;image.signature[2]='M';image.signature[3]='X';image.signature[4]=2;image.body_bytes=size;image.body_checksum=body_sum;image.header_bytes=32;image.reserved=0;image.header_checksum=sum((unsigned char*)&image,30);memcpy(prefix,&image,32);block=32;}
 if(mxos_write(&output,prefix,block)!=block)goto write_error;
 left=size;while(left){block=left>sizeof(buffer)?sizeof(buffer):left;if(mxos_read(&input,buffer,block)!=block||mxos_write(&output,buffer,block)!=block)goto write_error;left-=block;}
 if(is_rks){putword(prefix,body_sum);if(mxos_write(&output,prefix,2)!=2)goto write_error;}
 f=mxos_fclose(&output);mxos_fclose(&input);if(f)return fail("CANNOT CLOSE TARGET");
 mxos_puts("SAVED ");mxos_puts(target);mxos_puts(" LOAD=");mxos_print_hex16(image.load);mxos_puts(" ENTRY=");mxos_print_hex16(image.entry);mxos_putch(10);return 0;
arguments:mxos_fclose(&input);return fail("INVALID METADATA OR ARGUMENTS");
bad:mxos_fclose(&input);return fail("INVALID SOURCE OR READ ERROR");
write_error:mxos_fclose(&output);mxos_fclose(&input);return fail("WRITE FAILED: TARGET IS INCOMPLETE");
}
