# G07 A2 — transplante rastreável do PICO-08

## Decisão anterior ao código

O operador reprovou fisicamente o candidato `3a37aed682877ac5e60546d6d5fc850a1dcc8cbd`: não pareou.
Não há captura HCI nem informação suficiente para atribuir a falha a um estágio.
Os testes anteriores não provam equivalência com o PICO-08 nem aceite físico.

A2 substitui a máquina nova por `classic_keyboard.c` e `.h` **byte a byte** de
`tiagooliveirajs/picow-mouse-remapper`, branch
`feat/pico-08-classic-keyboard-integration`, commit
`0d917e58e73acf4333d7bc773186f97898ee3ffa`.
O CI verificará hashes e compilação efetiva desse fonte no firmware.

Referências: `e7be432` (host Classic), `6b09417` (integração/coexistência),
`8fbb36f` (stack SRAM 8 KiB e início após LCD), `208a487` (reserva linker zero).
O relato do operador atesta a branch histórica; não inventamos aceite por SHA.

## Fronteira do transplante

- Preservar algoritmos, estados, retries, inquiry, remote-name, SSP/PIN,
  aceitação de conexões, descriptor, TLV do peer, reconnect e timer originais.
- Preservar seção crítica e comandos originais entre cores. Inicializar a
  interface compartilhada no Core0 antes de iniciar o Core1.
- Adaptar somente a borda de UI para o snapshot original, sem dar ao observador
  autoridade para parar, iniciar ou reconfigurar operações HCI.
- A fila histórica de relatórios termina num consumidor sem saída USB em A2.
  O parser original continua compilado; canonical/USB continuam no G06.
- Restaurar LCD renderizada com sucesso → 500 ms servindo USB/UI → lançamento
  Core1 com buffer SRAM de 8 KiB. Eliminar o sleep substituto dentro do Core1.
- Restaurar HIDS client init → Classic init → handlers BLE → HCI power-on.
- Preservar a política LE NO_INPUT_NO_OUTPUT do G06; o SSP Classic continua
  DISPLAY_ONLY, exatamente como o fonte histórico. Esta divergência protege
  o Mouse G06 e deve constar de qualquer avaliação de equivalência.
- Preservar SDK 2.2.0, target pico2_w/RP2350, uma instância BTstack, lock do
  async context e proteção multicore de flash verificada no ELF.
- Preservar o comportamento BLE G06 e seu filtro de handle de desconexão.
  Portanto A2 transplanta o host Classic, não todo o firmware PICO-08.

## Critério e limites

Sem watchdog/arbitragem nova. Não acrescentar correção especulativa ao fonte
histórico. Limitações históricas, inclusive cancelamento durante remote-name e
incoming/reconnect, permanecem observáveis. Não confundir sua preservação com
uma aprovação de produto. Observador não transforma erro assíncrono em retry.

Teste inicial: power-cycle, Mouse desligado, OTHER OPTIONS → PAIR KEYBOARD,
colocar BKB-3G em pairing. Registrar a mensagem literal, PIN e resultado.
Esperado: KEYBOARD CONNECTED / CONNECTION CONFIRMED. Em seguida repetir com
Mouse funcionando. Digitação USB ainda não pertence a A2.

PR #9 e #10 permanecem draft, sem merge. G07 não aceito; sem G08.

## Implementação e verificação local

O fonte e header históricos estão em `src/classic_hid/pico08/`; `provenance.json`
registra a origem e SHA-256. `tests/check_pico08_source.py` verifica os bytes.
A borda de UI está em `classic_probe_pico.c`; `pico08_compat/` contém somente o
consumidor sem saída USB e os IDs históricos de relatório. Não importa TinyUSB.

A sequência de bootstrap usa um callback de preparação HIDS separado dos
handlers BLE. `boot_gate.h` controla o lançamento após o primeiro frame,
inclusive timestamp zero e wraparound. A tela Pair identifica **PICO-08 A2**.
`INQUIRY RESULTS` conta respostas observadas, não dispositivos únicos.

15/15 testes host locais passaram (Release com asserts ativos), incluindo:
fluxos originais de nome ausente, rejeição de remote-name, retry de HID,
SSP/PIN, descriptor, cancel, armazenamento TLV e reconnect; sequência HIDS /
Classic / BLE / HCI; gate LCD e atraso; identidade dos fontes; contratos G06.
O fixture de cancelamento usa conclusão HCI posterior para inquiry ativa.
Não cobre o rádio nem todas as interleavings históricas.

O verificador de produção exige uma compilação do fonte original com BLE e
Classic habilitados e seus símbolos no ELF, além das verificações herdadas de
BTstack único e flash multicore. O manifesto do build incorpora a proveniência.
O resultado remoto, SHA final, run e hash UF2 serão registrados no PR #10 após
concluir o CI. Nenhum resultado físico novo foi obtido nesta execução.
