# Rotação fixa de movimento — experimental G06

## Base e comportamento

Branch: `experimental/g06-rotation-90`.
Base exata: G06 aceito, `7eee024ad4ee726c5a85ffa2f32b9f47187878af`, versão `0.6.0-g06`.
Hardware: Raspberry Pi Pico 2 W (RP2350), com o HAT da base G06.

Esta variante aplica permanentemente `USB X = -Y do mouse` e `USB Y = X do mouse`.
O mouse já usa essa orientação desde o primeiro movimento após parear ou reconectar.
Não há opção extra para ativar a rotação. Ela vale para Passthrough, Default,
Escape e Custom, inclusive após reiniciar e enquanto a tela está bloqueada.

| Movimento físico | Movimento do ponteiro |
| --- | --- |
| Esquerda | Cima |
| Direita | Baixo |
| Frente | Direita |
| Trás | Esquerda |
| Frente e direita | Direita e baixo |
| Frente e esquerda | Direita e cima |

Diagonais, curvas e círculos são transformados vetorialmente, sem limiar de
direção, zona morta, ganho ou seleção exclusiva de eixo. A rotação preserva
o comprimento do vetor; a aceleração do sistema operacional continua externa
ao firmware. Grandes deslocamentos continuam divididos em relatórios USB,
como na base G06, preservando o deslocamento total.

## Implementação

A transformação está na preparação do relatório USB em `usb_hid.c`, chamada
por `service_usb_mouse` em `main.c`. O parser BLE, o pareamento/bonding,
o reconnect, o tratamento Logitech HID++, os perfis de botões, a flash e as
telas continuam sendo os da base G06. Wheel/pan e botões não são rotacionados.
Nesta branch, a regra de X/Y inalterados da documentação original G06 passa
a significar inalterados pelo perfil de botões, seguidos desta rotação global.

A rotação ocorre antes de reduzir cada componente aos oito bits do USB.
A negação é feita em 64 bits, inclusive para `INT32_MIN`. Somente após envio
USB bem-sucedido são consumidos os deltas originais `(report.y, -report.x)`.
Assim, USB ocupado preserva os dados pendentes e valores como Y=-128 não
invertem o sinal por overflow. Nenhum deslocamento excedente é descartado.

## Validação automatizada

O teste `rotation_90_vectors_and_chunking` cobre 65.025 vetores pequenos,
trajetória circular, limites de 32 bits e 484 combinações de grandes deltas
nos quatro perfis, incluindo os limites de 16 bits e fragmentação USB.
Também confere wheel/pan, botões e reconstrução idêntica de relatório pendente.
A revisão do serviço USB confere retorno antes de consumir em caso de falha.

Os 13 testes host passam em Debug. Duas expectativas antigas do teste G02
foram corrigidas para a tela já presente no G06 aceito: `LEARN`, e coluna 15
dos rótulos KEY. Nenhuma tela foi alterada para acomodar os testes.
As assertions também ficam ativas no build Release do CI.
O CI desta branch produz o UF2 Pico 2 W com SDK/toolchain fixados em
`ci/toolchain.env`. A validação física continua pendente do usuário.

## Testes físicos

1. No computador, pare o preset de rotação do Input Remapper e desative seu
   autoload. Se aplicou uma matriz pelo xinput, restaure a matriz original.
   Aplique a rotação apenas no firmware para não somar duas rotações.
2. Grave o UF2 desta branch usando BOOTSEL no Pico 2 W. Não é necessário
   executar apagamento total da flash como parte desta instalação.
3. Faça o pareamento normal da base G06. Se já houver bond compatível, teste
   primeiro a reconexão, mantendo o mouse no canal usado anteriormente.
4. Desde o primeiro movimento, confira as quatro direções da tabela.
5. Faça diagonais suaves e círculos completos, em ambas as direções, e
   movimentos muito pequenos. Ambos os eixos devem atuar simultaneamente.
6. Faça movimentos rápidos e longos. Não deve haver inversão inesperada.
7. Confira cliques, arraste, scroll e, no Lift, o Forward mantido em um perfil
   com remapeamento. A rotação deve continuar durante o arraste.
8. Troque os perfis de botões: todos devem manter a rotação. Bloqueie a tela
   e confirme que o mouse continua operando nessa orientação.
9. Desligue e religue o Pico e o mouse. Confirme reconexão com a rotação ativa
   sem reaplicar configurações e restauração do perfil de botões.

Para voltar ao comportamento anterior, grave o UF2 do G06 original.
