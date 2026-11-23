"""Initialize a DOS bank for CPU fixtures that enter below kernelCold."""


def initialize_dos_state(memory, symbols):
    start = symbols['dosWorkStart']
    defaults_end = symbols['dosDefaultsEnd']
    source = symbols['dosColdDefaults']
    end = symbols['dosZeroEnd']
    memory[start:defaults_end] = memory[source:source + defaults_end - start]
    memory[defaults_end:end] = bytes(end - defaults_end)
