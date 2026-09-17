# G07 reconstruído — Experimento A: conexão

Este candidato parte de G06 `7eee024ad4ee726c5a85ffa2f32b9f47187878af`.
A investigação anterior à implementação está em `08-g07-rebuild-investigation.md`.
PR #9 / branch antiga permanecem preservados. Sem merge, sem G08.

## Escopo exato

Apenas conexão Classic e convivência com Mouse BLE G06. **Não encaminha ainda
teclas do teclado físico à USB.** O teclado USB fixo continua disponível para
Escape sintético do remap G06. Não declarar G07 concluído nem executar ainda
R4/typing ou G07-01…10 como aceite final.

O candidato é deliberadamente uma etapa de conexão, não uma firmware de debug
com CDC/UART. O mesmo target de produção `pico2_w` e a identidade USB G06 são
mantidos. O campo de versão base permanece 0.6.0-g06 durante o experimento:
identifique o candidato pelo SHA/hashes do artifact, não por esse campo.

## O que mudou e por quê

- `classic_probe_pico.c/.h`: Inquiry de 5 unidades, lista de 20 dispositivos,
  EIR/nome remoto, dois nomes exatos, REPORT HID, SSP display-only/legacy 0000,
  descriptor Keyboard. Derivado semanticamente de `classic_keyboard.c` do
  PICO-08 `0d917e5`, não da máquina acumulada em G07.
- `bt_runtime_pico.c/.h`: composição explícita de dois callbacks, sem registry.
  Core1 com stack SRAM de 8 KiB; callbacks antes de power-on; flash-safe em
  ambos os cores. Setup protegido do worker background do SDK.
- `ble_hogp_pico.c/.h`: expõe o setup para composição e filtra disconnect pelo
  handle Mouse. Discovery, bonded reconnect, HID++ e parser G06 preservados.
- CMake/config: uma compilação da base BTstack, headers dos adapters com os
  dois feature defines; capacidade para dois ACLs e Classic keys. **LE DB
  continua com os 8 slots aceitos no G06**; não importar mudanças não necessárias.
- `app/main.c`: um comando Pair/Cancel e snapshot de conexão/PIN, sem transportar
  reports. Nenhuma tela dispara Bluetooth em polling. Help mantém Pair;
  sair da região Pair/Help cancela tentativa, mas não desconecta READY.
- `tests`: adapter de produção com eventos determinísticos e bootstrap real
  com seams de plataforma; compile commands/ELF auditados no CI.
- Flash SDK: `pico_flash/flash.c` também precisa ser materializado uma única vez,
  no runtime que liga `pico_multicore`. O SDK condiciona lockout a
  `LIB_PICO_MULTICORE`; compilar outra cópia no archive storage apenas com
  `pico_flash` gera uma variante sem lockout. A seleção do linker não deve
  decidir essa política. Storage usa os headers; a imagem final compartilha
  a implementação multicore. CI verifica objeto único e o define. Isso corrige
  um risco demonstrável de composição, não prova a causa do pairing físico.
- CI: asserções ligadas também em Release. G06 tinha `assert()` desativado e
  expectativas obsoletas no teste G02 ("LEAR" e coluna 15 em vez de 16).
  Ajustados somente os testes ao contrato G03 já aceito; produto não mudou.

## Divergências conscientes do histórico

1. Mantém LE SM NO_INPUT_NO_OUTPUT de G06; PICO-08 usava DISPLAY_ONLY em SM.
   Classic SSP é DISPLAY_ONLY em ambos. Preserva política do Mouse aceito.
2. Callback Classic antes do setup BLE; ambos depois de L2CAP/SM/GATT/ATT e
   antes de HCI power. PICO-08 inicializava HIDS antes de Classic, registrava o
   handler BLE depois. Aqui HIDS+handler BLE permanecem no mesmo setup G06.
3. Nenhuma pausa BLE, cancelamento LE ou retry de erro foi importado. Inquiry
   vazia/nome não alvo repete o ciclo histórico; uma rejeição é exibida.
4. API success não vira SEARCHING até HCI ACK. Cancelamento antes de ACK espera
   ACK e então cancela. Cancel muda intenção antes de stop potencialmente
   reentrante; operações pendentes não são reutilizadas.
5. Incoming Classic só é aceito durante Pair explícito. Não introduz registry
   Keyboard, preferred/reconnect automático ou TLV KB3G de outro gate.
6. READY exige descriptor com usage page Keyboard. Não apenas canal aberto.
7. PIN/confirmation de outro endereço não responde pela sessão atual.
8. UI fica em PAIR KEYBOARD mostrando KEYBOARD CONNECTED; não mostra
   KEYBOARD SAVED nem afirma que um registry foi atualizado.

## Limites e diagnóstico

Sem controlador físico nesta execução. Não há retry/watchdog por fase no
experimento A. Se um comando não produzir completion, o último estágio
permanece visível: registrar a tela após 60 segundos e power-cycle antes da
próxima tentativa. Cancel não reseta o controller compartilhado nem o Mouse.
`FINISHING REQUEST` significa que uma operação anterior ainda não terminou;
não empilhar Pair. Esse limite evita declarar uma transação viva como livre.

Estados visíveis: STARTING BLUETOOTH → STARTING SEARCH (aguarda ACK) → SEARCHING
KEYBOARD → READING DEVICE NAME, se necessário → CONNECTING KEYBOARD → PIN,
se exigido → SETTING UP KEYBOARD → KEYBOARD CONNECTED.

Erros 00–FF vindos de API/HCI são exibidos literalmente. E1 é passkey-input
incompatível com host display-only; E2 é descriptor sem Keyboard. Falha de
bootstrap pode deixar STARTING BLUETOOTH. Nenhuma dessas telas prova causa raiz.

## Testes físicos desta etapa

Use somente o UF2 do artifact cujo `commit`, tamanho e SHA-256 correspondem ao
manifesto `g07-experiment-a-evidence.json`. Não apagar flash/bonds como passo
prévio genérico; preservar persistência do G06.

1. **R1:** power-cycle, Mouse desligado, HOME → OTHER OPTIONS → PAIR KEYBOARD,
   BKB-3G em pairing. Se aparecer PIN, digitá-lo no BKB e Enter. Sucesso nesta
   etapa é **KEYBOARD CONNECTED / CONNECTION CONFIRMED** na LCD.
2. **R2:** power-cycle, Mouse conectado e funcional antes de Pair Keyboard.
   Confirmar a mesma conexão de R1 sem perda de movimento/click/scroll Mouse.
3. **R3:** conectar Keyboard primeiro, depois ligar Mouse. Ambos conectados;
   Mouse funcional. A ainda não envia typing do Keyboard para o PC.
4. **R5 parcial:** desligar Keyboard e verificar Mouse funcional. A tela Pair
   deixa CONNECTED; não se exige reconnect Keyboard automático.
5. **R7:** Pair sem alvo, Key B. UI/HAT e Mouse devem continuar funcionais;
   Help não dispara nova busca; voltar e tentar Pair novamente após drenagem.
6. Regressão G06: perfil/persistência, Lift HID++, Custom/remap/Escape,
   Learn/lock/unlock e USB fixo devem continuar funcionando.

Registrar por teste: SHA do candidato, ordem dos dispositivos, resultado,
última fase, código de erro e FOUND, PIN apareceu ou não, Mouse permaneceu
funcional ou não. **Se R1/R2 falharem, não iniciar B/D.** Se passarem, próxima
etapa B adiciona canonical Keyboard/ownership/typing em commit separado,
seguida pela confirmação de R1–R7 e somente depois UX completa D e G07 final.

## Reproduzir validação

Host: `cmake -S . -B build-host -DBLU2USB_BUILD_PICO=OFF -DBLU2USB_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release`,
`cmake --build build-host --parallel`, `ctest --test-dir build-host --output-on-failure`.
Produção: Pico SDK 2.2.0, pacote GCC `15:13.2.rel1-2`, opções da workflow,
`PICO_BOARD=pico2_w`; depois `python3 tests/check_g07_production_image.py build-pico`.
O manifesto e compile_commands/map/ELF pertencem ao mesmo build que gerou UF2.
Testes host e CI não substituem aceite físico.
