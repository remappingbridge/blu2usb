# BLU2USB-G07 — comparação histórica do runtime Classic HID

## Motivo

O candidato G07 `828271deedd0ca0d31b726e5e4fb236e835399f1` adicionou tratamento para rejeição assíncrona de HCI Inquiry, watchdogs por fase e diagnóstico visual. Mesmo assim, o BKB-3G fisicamente comprovado continuou sem conectar, tanto sozinho quanto com Mouse BLE.

A investigação seguinte comparou o G07 atual com as implementações do `tiagooliveirajs/picow-mouse-remapper` entre PICO-06/PICO-07/PICO-08, a POC `poc/bkb3g-classic-hid` e a reimplementação posterior `gate/g02-classic-hid-integration`.

## O que não é o mesmo bug

### PICO-07 BLE Keyboard

O PICO-07 antigo ainda tentava descobrir o teclado pelo fluxo BLE HOGP. O teste físico registrou Mouse PASS e Keyboard FAIL. As correções daquele fluxo foram específicas de LE discovery: scan ativo, janela maior, aceitação por UUID/Appearance, evitar corrida com o Mouse preferido e ampliar o Report Map buffer.

O BKB-3G usado neste projeto foi depois isolado por uma POC que provou que o transporte correto é Bluetooth Classic HID Host. O G07 atual já usa BR/EDR Classic; portanto os filtros BLE do PICO-07 não são uma correção aplicável a este bug.

### Nome anunciado pelo BKB-3G

A POC antiga inicialmente esperava apenas `BKB-3G`. O commit histórico `eaf3ecd5e2e72225eea7eb9bdf7d7b4a909f1443` corrigiu o alvo primário para `Bluetooth keyboard 3.0` e preservou `BKB-3G` como alias.

O G07 atual já usa exatamente os dois nomes e o mesmo matching exato. Logo a regressão física atual não é a repetição desse bug de nome.

### Sizing de BTstack

A POC e o G02 posterior usam, entre outros limites, duas conexões HCI, uma conexão HID Host, quatro canais L2CAP, três serviços L2CAP e link-key DB persistente. O G07 já havia sido alinhado a esses requisitos antes do candidato Astra. A persistência/sizing não explica por si só a falha restante.

## Invariante presente em todas as implementações Classic que funcionaram

A diferença material restante é o contexto de execução.

### POC BKB-3G

A POC `poc/bkb3g-classic-hid` executa o Bluetooth em Core1:

1. `cyw43_arch_init()`;
2. inicialização L2CAP/HID Host/GAP/HCI;
3. `hci_power_control(HCI_POWER_ON)`;
4. `btstack_run_loop_execute()`;
5. TinyUSB permanece no outro core.

O histórico inclui inclusive `POC: run BTstack and TinyUSB on separate cores` e depois move o entrypoint Bluetooth para uma unidade separada da TinyUSB.

### PICO-08 BLE Mouse + Classic Keyboard

O PICO-08 preservou BLE e Classic no mesmo owner Core1. Durante integração houve uma falha física de runtime. O projeto tentou ampliar o stack padrão de Core1 para 8 KiB, encontrou conflito com a região linker-owned do RP2350 e terminou com a solução comprovada:

- `PICO_CORE1_STACK_SIZE=0`;
- buffer SRAM alinhado explícito de 8 KiB;
- `multicore_launch_core1_with_stack()`;
- startup Bluetooth somente após a UI inicial, com mais 500 ms de estabilização;
- `btstack_run_loop_execute()` no Core1.

Os commits centrais desse ajuste foram `4a42bd7387274594b8c0d75689bbdff6e442c338`, `8fbb36f3e551bafabb038afdf7e8441eab4ea9a8` e `208a487856f1f4a8ffb2cf2f44764621370e10e0`.

A falha documentada nessa sequência foi de runtime/UI, não foi provado que seja a mesma causa física do não-pareamento atual. O valor da evidência é arquitetural: o workload BLE+Classic fisicamente integrado acabou congelado nesse envelope de execução.

### Reimplementação G02/G03/G04 posterior

A linha posterior `gate/g02-classic-hid-integration` integrou novamente o Classic Host a partir da POC. `ClassicHidHost.c` continua chamando `cyw43_arch_init()`, inicializa o HID Host, liga HCI e termina em `btstack_run_loop_execute()`. `main.c` lança `BT_HOST_CoreMain` em Core1 enquanto TinyUSB permanece em Core0. O G03 seguinte declara explicitamente que as operações BTstack permanecem em Core1.

Portanto Core1 + run loop dedicado não foi uma peculiaridade temporária da primeira POC: foi mantido nas duas linhas de implementação Classic posteriores.

## Divergência do candidato G07 que falhou

O candidato `828271de...` era a exceção histórica:

- inicializava CYW43/BTstack no Core0;
- usava `pico_cyw43_arch_threadsafe_background` como único mecanismo de serviço contínuo;
- não chamava `btstack_run_loop_execute()`;
- não possuía stack dedicado para o workload BLE+Classic.

O SDK suporta timers e pending work pelo async context, portanto essa diferença isoladamente não prova a causa física. Porém, depois que name matching, sizing, radio arbitration, rejection handling e timeouts foram alinhados e o dispositivo ainda não conectou, não há justificativa para continuar divergindo do único envelope Classic fisicamente comprovado.

## Correção G07

A partir desta revisão, o G07 preserva a máquina de estados robusta adicionada pelo Astra, mas move o runtime Bluetooth compartilhado para o envelope conhecido:

- Core0 continua proprietário de USB, HAT, LCD e aplicação;
- BLE HOGP Mouse e Classic HID Keyboard continuam compartilhando uma única imagem BTstack/CYW43;
- todo o runtime Bluetooth é iniciado em Core1;
- Core1 recebe buffer SRAM alinhado explícito de 8 KiB;
- `PICO_CORE1_STACK_SIZE=0` evita a reserva linker-owned que causou problema no PICO-08;
- startup do Core1 ocorre somente depois que `main` já inicializou USB/HAT/LCD, com 500 ms adicionais dentro do próprio Core1;
- Core1 executa `btstack_run_loop_execute()`;
- o async context threadsafe-background continua sendo a integração CYW43 do SDK, mas deixa de ser o único owner de progresso do BTstack;
- os dois cores registram suporte a `flash_safe_execute`, porque estado do produto pode ser persistido por Core0 e credentials BTstack por Core1;
- comunicação Bluetooth -> aplicação continua pela fila atômica já existente;
- comandos Pair/Retry/Cancel continuam mailboxes atômicos; não há chamada BTstack bruta no Core0;
- toda proteção do Astra para rejeição assíncrona, watchdog, CID/event ownership e diagnóstico LCD é mantida.

## Limite da conclusão

Não se declara que o bug físico atual é exatamente o mesmo defeito dos commits de stack do PICO-08. O histórico prova três fatos mais restritos:

1. o PICO-07 BLE discovery failure era outro problema e não se aplica ao Classic atual;
2. o name mismatch antigo já está corrigido no G07;
3. todas as implementações Classic conhecidas que funcionaram no RP2350 mantiveram BTstack em Core1 com `btstack_run_loop_execute()`, e a integração BLE+Classic comprovada terminou usando stack explícito de 8 KiB.

O novo UF2 deve ser tratado como candidato físico. G07 permanece aberto/draft até os testes reais passarem.

## Teste físico prioritário

Antes da suíte completa, validar:

- **R1 — Keyboard sozinho:** power-cycle, Mouse desligado, `PAIR KEYBOARD`, BKB-3G em pairing. Registrar fase/erro/contadores se falhar.
- **R2 — Mouse primeiro:** Mouse BLE conectado e funcional; iniciar Pair Keyboard. Keyboard deve conectar sem derrubar Mouse.
- **R3 — Keyboard primeiro:** parear Keyboard e só depois ligar Mouse. Ambos devem permanecer funcionais.
- **R4 — Cancelamento:** Pair Keyboard sem alvo e Key B. O Core1 não pode bloquear USB/HAT e o Mouse discovery deve retomar.

Somente depois desses quatro cenários passam-se T1–T7 e G07-01…G07-10 do documento principal.
