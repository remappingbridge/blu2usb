# G07: investigação e reconstrução controlada desde G06

Data: 2026-09-17. Documento escrito antes das alterações de firmware.

## Decisão e limite da evidência

**Reconstruir desde G06 (opção B), em uma branch separada.** Não alterar nem
reescrever `gate/g07-keyboard-transport-classic-hid`; PR #9 permanece aberto,
draft e sem merge. Nenhum avanço a G08.

A falha física persistente é relatada pelo operador no pedido desta execução.
Não temos captura HCI do hardware: **a causa raiz física permanece não
demonstrada**. A investigação localiza a descontinuidade arquitetural, não um
commit comprovadamente culpado. Os sete grupos de correções anteriores não
constituem sete provas da causa; vários corrigem defeitos reais posteriores à
fase onde o pareamento pode estar falhando.

O primeiro candidato será **experimento A, conexão**, sobre o G06 exato,
com bootstrap compartilhado explícito e algoritmo Classic derivado do PICO-08.
Preservará Mouse, HID++, perfis, storage, HAT e USB. Não implementará ainda o
novo encaminhamento físico Keyboard→canonical→USB, a facade completa, registry
de teclados nem reconexão persistente do teclado. A tela existente terá apenas
estado/PIN suficientes para executar o experimento sem terminal. Isso é uma
adaptação de observação necessária, não a UX completa do experimento D.

**B/C/D dependem da prova física de A.** Não publicar um firmware final G07
como se essa condição já estivesse satisfeita. No A, R1/R2/R3/R7 avaliam conexão,
coexistência e cancelamento; R4/typing e R6/Keyboard→USB aguardam B. R5 pode
avaliar isolamento da desconexão do teclado mantendo o Mouse.

## Fontes e referências congeladas

| Fonte | Ref inspecionada | Significado |
|---|---|---|
| `blu2usb-RP2350` | `7eee024ad4ee726c5a85ffa2f32b9f47187878af` | G06 fisicamente aceito pelo operador; base de produto |
| `blu2usb-RP2350` | `f14c108c5f053486305753411c652643f3482f42` | G07 que continuou falhando, PR #9 |
| `picow-mouse-remapper` | `0d917e58e73acf4333d7bc773186f97898ee3ffa` | head PICO-08, PR #7, referência de BLE+Classic |
| `picow-mouse-remapper` | `5f08260fc750e7f909bcda19b6880457ca3d9e31` | POC BKB-3G |
| `picow-remapper` | `341481f07ab35a49fd27e60901c9f11657e98440` e linha G06 | migração intermediária para runtime BLE Core0 |
| `infra-planner` | `22a168041f4f42ff9ee958e12fdc0baf82b57e24` | planejamento atual consultado |
| Pico SDK | `a1438dff1d38bd9c65dbd693f0e5db4b9ae91779` (2.2.0) | integração CYW43/run loop/TLV |
| BTstack | `501e6d2b86e6c92bfb9c390bcf55709938e25ac1` | semântica real de Inquiry/HID |

O operador identifica a branch PICO-08 como funcional simultaneamente. O PR #7
tem head `0d917e5`; sua discussão retornou vazia. Não foi encontrada uma ata
independente vinculando o teste físico a esse SHA específico. Usamos o head
como referência de código, sem inventar um aceite físico por SHA.

### Divergência de planejamento

`infra-planner/docs/19-picow-remapper-implementation-gates.md` diz "planned only"
e `plan/picow-remapper-work-items.cue` ainda tem W10.* propostos, todos os gates
insatisfeitos. Lá Classic é REMAPPER-G08; no BLU2USB atual é G07, pois Mouse e
HID++ foram reorganizados em G05/G06. Não confundir os números nem marcar
W10.8 aceito. O pedido atual autoriza a reconstrução do **BLU2USB-G07**.

## Timeline: bugs diferentes, linhas diferentes

1. PICO-06 (`5b22cad`): remap/UX aceitos conforme PR #5. Não prova Classic.
2. PICO-07 (`5b93ef7`): UI type-first/Saved Devices e scan BLE; PR #6 registra
   Mouse PASS e Keyboard FAIL. `55033a5`, `a9c4bdf`, `21ebfca`, `3f3f4bd` e
   `5b93ef7` aumentam janela/filtros/preferred-peer. Transporte errado para BKB.
3. POC: `bad97f7` implementa Classic; `45ce044` fixa Pico 2 W/RP2350;
   `eaf3ecd` inclui `Bluetooth keyboard 3.0` além de `BKB-3G`; `38036ea` separa
   BT/TinyUSB em cores; `5f08260` separa translation units pelos tipos HID.
4. PICO-08: `e7be432` importa Classic; `6b09417` registra BLE+Classic antes de
   HCI power; `d568cdc` compõe build; `48bb1a9` integra saída USB estável;
   `a7aefe2` faz Keyboard UI chamar Classic. São mudanças semânticas distintas.
5. `4a42bd7` volta à reserva Core1 de 4 KiB depois do conflito de linker;
   `8fbb36f` introduz buffer SRAM explícito de 8 KiB e atraso pós-LCD;
   `208a487` zera reserva linker-owned; `0d917e5` muda apenas label provisional.
   Esses commits tratam runtime/stack/LCD, não demonstram a causa física atual.
6. `picow-remapper`: especificação nova e implementação modular. `341481f` e
   `cf49377` mudam runtime BLE para Core0/background após problemas de Core1.
   A linha não reintroduz Classic. A migração não é uma simples renomeação.
7. BLU2USB: `0975f17` congela contratos, `fd2b98a` refaz UX por comandos/release,
   `5f1dae2` inicia HAT; G03 corrige geometria/navegação; G04 fixa USB;
   `6dbacf4` implementa runtime BLE Core0; G06 agrega profiles/HID++/persistence,
   terminando em `7eee024`. **Classic funcional não está presente nesse G06.**
8. G07: `439eb43` reimplementa adapter; `5c887f0`/`9807fc9` adicionam registry
   de setups; `e477d9d` liga facade a comando/saída; depois vêm os patches abaixo.

O ponto de perda localizável é a **reimplementação entre PICO-08 e as novas
linhas modulares**, onde o Classic deixou de fazer parte do firmware. Não há
evidência de que um botão isolado tenha quebrado um Classic previamente aceito
no BLU2USB. O G07 é a primeira reintegração ainda não aceita.

## Lições das tentativas G07

| Commits/grupo | O que realmente muda | Lição para reconstrução |
|---|---|---|
| `d528052`, `9a9d96f` | pools/links/keys/L2CAP | capacidade necessária; não prova execução de Inquiry |
| `a4a74fb` | REPORT em lugar de fallback | preservar REPORT; diferença só depois da descoberta |
| `f539a05` | centraliza fontes BTstack num archive | auditar compile commands e ELF; compilar não prova composição |
| `4875b80`, `1e8cab2` | pausa BLE, cancel LE, retry | não importar arbitragem como causa provada; PICO-08 não a tem |
| `1711a8f` | filtra disconnect pelo handle Mouse | reter esse isolamento necessário à coexistência |
| `828271d` | Command Status, deadlines, drain, progress | API success não é ACK HCI; preservar observação desse erro sem empilhar retries |
| `330fc2c`…`f14c108` | Core1 explícito, run loop, flash coordination | já falhou fisicamente; Core1 sozinho não é correção demonstrada |

O harness existente cobre transições artificialmente injetadas; não comprova
radio, controller ACK, callbacks da imagem final nem o caminho HAT→comando.
Novos testes devem incluir bootstrap/composição, comandos antes de WORKING,
cancelamento reentrante e isolamento por handle, além de hardware.

## Matriz comparativa

Legenda: F = diferença funcional; C = aparentemente cosmética; N = necessária
à nova arquitetura; R = potencial regressão; E = relevância demonstrada em
código/histórico (não equivale a causa física provada).

| Área | G06 aceito | G07 f14c108 | PICO-08 | POC | Classificação/efeito |
|---|---|---|---|---|---|
| CYW43 init | runtime Core0 | runtime Core1 | `picow_bt_example_init` Core1 | `bkb3g_classic_hid_core_main` | F/R; comparar contexto completo |
| BTstack init | SDK via CYW43 | mesmo SDK | mesmo SDK | mesmo SDK | E; memory/run-loop/HCI/TLV vêm do SDK |
| Core owner | background Core0 | background Core1, 8 KiB | Core1, 8 KiB explícitos | Core1 default | F/R; stack não determina sozinho causa |
| run loop | async background | `btstack_run_loop_execute` | execute | execute | F; execute usa mesmo async context |
| HCI power | depois BLE setup | depois BLE+Classic setups | depois ambos | depois HID/GAP | E; todos power uma vez |
| BLE init | SM/GATT/ATT/HIDS | idem | idem, SM DISPLAY_ONLY | SM, sem HOGP ativo | F; manter Mouse G06, documentar IO |
| Classic init | ausente | callback via registry | chamada explícita no bootstrap | explícita | N/R; remover indireção no A |
| L2CAP init | runtime | runtime | `btstack_main` | `bkb3g_classic_hid_init` | E; exatamente uma chamada |
| SDP init | sem servidor | HID client, sem `sdp_init` | igual | igual | ausência não é regressão; consulta client |
| SM init | NO_INPUT_NO_OUTPUT | idem | DISPLAY_ONLY | defaults | F/R; SSP tem configuração distinta |
| HID Host init | ausente | storage 1024 | storage 1024 | storage 1024 | F/E; dois PSM registrados pelo SDK |
| HCI handlers | BLE+SDK | BLE, Classic, SDK | Classic, BLE, log, SDK | Classic, SDK | N/R; ordem diferente, static lifetime |
| Inquiry | ausente | 5×1,28s, ACK/deadline/retry | 5×1,28s | 5×1,28s | F/R; novas guardas no G07 |
| Remote name | ausente | serial, index/deadline/drain | serial | serial | E; EIR ausente é caso necessário |
| Target matching | ausente | dois nomes exatos | dois nomes exatos | dois nomes após eaf3ecd | E; sem filtro COD/UUID alternativo |
| SSP | BLE SM | Classic DISPLAY_ONLY, confirmação | igual | igual | E; passkey exibida, não digitada no Pico |
| Legacy PIN | ausente | 0000 apenas CONNECTING | 0000 | 0000 | F/R; guarda nova |
| Link keys | NVM=0 | SDK TLV, NVM=16 | SDK TLV, NVM=16 | idem | F/E; manter sizing/SDK |
| Pair command | UX enum sem backend | app→facade→atomics→timer | UI→critical-section mailbox→timer | auto em WORKING | N/R; testar caminho real |
| Cancel | navegação apenas | state/drain/stop/disconnect | mailbox/stop/disconnect | sem UX cancel | N/E; estado antes de stop síncrono |
| Saved devices | Mouse bonded/profiles | Keyboard registry futuro | peer Keyboard KB3G TLV | descoberta automática | F; não importar registry de outro gate |
| BLE arbitration | Mouse reconnect/scan | pause/cancel/resume explícitos | independente | nenhum Mouse ativo | F/R; retirar no A |
| HCI connections | 1 | 2 | 2 | 2 | E; preservar mínimo 2 |
| Flash/TLV | storage produto + SDK | flash_safe ambos cores | Core0 lockout + SDK | SDK | N/R; preservar regiões independentes |
| Core communication | fila atômica callback→app | fila + 3 atomics de comando | snapshots/queues com critical section | fila raw | N; no A comando único + snapshot |
| UI interaction | release, Learn/lock | release + facade + watch 90s | provisional type-first | auto/log | N/R; nada de polling disparando pair |

Configuração de Inquiry: `hci_set_inquiry_mode(INQUIRY_MODE_RSSI_AND_EIR)`;
SDK envia Inquiry com número de respostas 0 (sem limite HCI); adapters retêm
20 dispositivos. Cache local é por endereço; EIR com nome não alvo é marcado
resolvido, EIR ausente vai para remote-name. Guardam page-scan repetition e
clock offset, passando bit 15 para uso do offset. Não há fallback por CoD,
serviço HID ou substring `GT T1` nos três adapters históricos comparados.

SSP: POC/PICO-08/G07 respondem confirmation e PIN 0000, recebem passkey
notification; não implementam USER_PASSKEY_REQUEST como entrada local.
Nenhum deles usa authentication-complete/pairing-complete como condição única
de READY: é necessário HID descriptor. Uma falha antes do PIN pode ser
discovery, name, ACL, SDP ou security; o relato não distingue essas fases.

## Call graphs reais, lado a lado

| Etapa | PICO-08 (arquivos em src_fw/picow_ble_usb_hid_bridge) | G07 (arquivos em src) |
|---|---|---|
| Boot | `main` → `classic_keyboard_shared_init` → `usb_dev_main` | `main` → ux/aggregator/profiles/restore/remap → USB/HAT/ST7789 |
| Lançamento | `maybe_start_bluetooth_core` → `multicore_launch_core1_with_stack(ble_host_main)` | HID++ start → `blu2usb_keyboard_transport_pico_start` → register; `blu2usb_ble_hogp_start` → runtime start → Core1 |
| CYW43 | `ble_host_main` do **hog_host_demo_poc.c** → `picow_bt_example_init` | `blu2usb_bt_core1_main` → `cyw43_arch_init` |
| Protocolos | `picow_bt_example_main` → `btstack_main` → L2CAP/SM/GATT/ATT/HIDS → `classic_keyboard_core1_init` | L2CAP/SM/GATT/ATT → `ble_hogp_session_setup` → `classic_hid_session_setup` |
| Power | `hci_power_control` → `btstack_run_loop_execute` | `hci_power_control` → `btstack_run_loop_execute` |
| WORKING | `packet_handler` Classic → APP_IDLE / saved reconnect; BLE handler | `packet_handler` Classic → stack_working / IDLE; BLE handler |
| Pair | `test_begin_pairing` → `test_start_scan` → `classic_keyboard_request_pair` | `blu2usb_ux_input` release → `handle_ux_command` → `blu2usb_keyboard_transport_pico_pair` → `blu2usb_classic_hid_pico_pair_keyboard` |
| Mailbox | `process_commands_and_reconnect` → `consume_pair_request` | `command_timer_handler` → `service_command_requests` |
| Inquiry | `start_inquiry` → `gap_inquiry_start` | `start_inquiry` → BLE pause predicate → `gap_inquiry_start` → ACK handler |
| Nome | `handle_inquiry_result` ou `request_next_remote_name` → `handle_remote_name_complete` | mesmos nomes locais, mas índices/deadlines/guards adicionais |
| Connect | `connect_target` → `hid_host_connect(REPORT)` | `connect_target` → `hid_host_connect(REPORT)` |
| Security | `packet_handler` PIN/SSP/passkey | `packet_handler` PIN/SSP/passkey |
| Descriptor | `handle_hid_meta` → save_peer → snapshot READY | HID meta → `descriptor_has_keyboard` → runtime CONNECTED |
| Reports | `normalize_and_enqueue_report` → keyboard queue → USB | `handle_hid_report` → canonical diff → runtime queue → facade decode → aggregator → USB |

**Armadilha de inspeção:** o CMake PICO-08 compila `hog_host_demo_poc.c`, que
inclui `hog_host_demo.c` com macros e substitui `ble_host_main`. Ler só o arquivo
base produz um call graph incompleto. Esse wrapper e suas macros não serão
copiados para o novo produto.

## Bootstrap, composição e UX auditados

G07 registra Classic antes de `blu2usb_ble_hogp_start`. `runtime_reset` limpa
somente a fila, não o registry. Os setups, handlers, timers e buffers têm
lifetime estático. Não foi encontrado segundo `cyw43_arch_init`, `l2cap_init`
ou reset que apague os registros depois de power-on. Os handlers existem no
código; sem execução física isso não prova sua entrega no firmware real.

PICO-08/POC compilam stack, adapters e integração num target final com BLE e
Classic habilitados. G06/G07 usam archives. G06 materializa BLE em mais de um
archive; `f539a05` centraliza BTstack no runtime, adapters usam headers. O
novo A centralizará fontes SDK Bluetooth e tornará os dois feature defines
consistentes para todos os consumidores de headers. Exportar compile commands
e linker map permitirá verificar número de objetos/definições. GATT local
permanece o do G06; TinyUSB e seu descriptor não mudam. Target permanece
`pico2_w`, SDK 2.2.0, GNU Arm 13.2.Rel1, conforme CI.

Na UX nova, press/release é decidido no interaction engine. Entrada em
OTHER OPTIONS não inicia pareamento; seleção de PAIR KEYBOARD em release
emite um comando. Polling de runtime não repete Pair. Help não cancela;
saída da página Pair para outra página cancela se teclado não conectado.
Unlock a partir de Help e watchdog independente podem deixar diferenças
laterais; não há prova de que expliquem a falha inicial em todos os testes.
`274b80f` corrige Back de KEYBOARD SAVED, após conexão — não descoberta.

## Máquina G07 auditada

| Estado | Entrada | Saída/evento | Timeout/retry/cancel |
|---|---|---|---|
| WAITING_FOR_STACK | session setup / Pair antes WORKING | WORKING → IDLE | Pair ativo: 15s → ERROR; cancel → IDLE |
| IDLE | WORKING, retry, close, cancel | timer + active + sem operações → Inquiry | radio 15s; retry 1s; pausa BLE pode reter |
| INQUIRY | start aceito pela API | HCI status / GAP complete / nome EIR alvo | 10s; 3 rejeições → ERROR; ACK separado |
| RESOLVING_NAMES | GAP complete | name complete → próximo / connect / Inquiry | 10s; erro → ERROR; índice pendente drena |
| CONNECTING | target encontrado / incoming | opened → descriptor → READY; failure → IDLE | 30s; PIN estende 60s; close/cancel pendentes |
| READY | descriptor de teclado | connection closed → IDLE | sem deadline ativo; Pair ignorado |
| ERROR | deadline / rejeições / drain | novo Pair/Retry se não há operações, ou close | nenhuma recuperação que resete controlador |

Combinações problemáticas: ERROR/IDLE com `g_name_index>=0` sem completion
impede nova tentativa (F1); Inquiry submitted sem ACK tem cancel diferido;
conexão entrante enquanto estado ERROR é recusada. São guardas intencionais
contra reuso de transações vivas, mas ampliam muito a distância do fluxo
histórico. Sem rastreio HCI não sabemos se modelam o caso físico ou escondem
uma falha anterior. Não transportar toda essa máquina para A.

## Plano verificável de reconstrução

1. Commit apenas desta investigação desde G06, preservando f14c108 remoto.
2. Experimento A: bootstrap explícito BLE+Classic, uma instância HCI/L2CAP;
   runtime Core1 com stack histórica; Inquiry/name/REPORT/SSP/PIN do PICO-08;
   nenhum BLE pause/retry coordinator, facade ou Keyboard registry;
   snapshot mínimo de conexão/PIN e comando único Pair/Cancel;
   filtro de disconnect Mouse por handle. Erros HCI observáveis, sem retry
   automático de rejeições. Cancel muda intenção antes de APIs reentrantes.
3. Host regressions G03–G06 + adapter/bootstrap tests; produção RP2350;
   verificar compile commands/map/CI no SHA; entregar UF2 de A com hash.
4. Aceite físico da conexão/coexistência de A. Se falhar, registrar fase exata
   e voltar à comparação; **não adicionar B/C/D para encobrir o resultado**.
5. B: canonical Keyboard com ownership e parsing; commit/candidato separado.
6. C: validar ambos em R1–R7; adicionar coordenação só se evidência a exigir.
7. D: facade/UX completa/status/Help, depois G07-01…10. Draft até aceite final.

Não apagar bonds nem setores de storage como tentativa genérica. Layout de
storage produto do G06 continua separado do TLV SDK. Sem alterações de VID
0xcafe, PID 0x4010, manufacturer BLU2USB, product BLU2USB Mouse + Keyboard,
interfaces Mouse=0/Keyboard=1; sem `tud_disconnect` ou re-enumeração.
