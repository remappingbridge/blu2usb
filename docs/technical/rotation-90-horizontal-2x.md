# Rotação 90° com ganho horizontal 2×

Branch: `experimental/g06-rotation-90-horizontal-2x`.
Base: `experimental/g06-rotation-90`, commit
`a8c4c1d2826d83c839c01797e34501ceeeac10e2`, testado fisicamente e aprovado
pelo usuário antes deste pedido. A branch original permanece intacta.

## Comportamento

Transformação fixa: `USB X = -2 * mouse Y`; `USB Y = mouse X`.
O ganho é de distância, sem aceleração dependente da velocidade.

| Movimento físico | Ponteiro |
| --- | --- |
| Frente | Direita, ganho 2× |
| Trás | Esquerda, ganho 2× |
| Esquerda | Cima, ganho 1× |
| Direita | Baixo, ganho 1× |

Um círculo físico torna-se uma elipse horizontal com largura duas vezes a
altura nos deltas USB. A aceleração/configuração do sistema operacional pode
alterar a trajetória visual; para comparar proporções, use resposta linear no
host. Diagonais e curvas continuam usando ambos os eixos simultaneamente.
A transformação está ativa desde a conexão, em todos os perfis, após reinício
e durante bloqueio da tela. Botões, wheel/pan, pareamento, reconnect e HID++
continuam com o comportamento da base aprovada.

## Pacotes e precisão

O acumulador guarda coordenadas originais. O construtor USB multiplica Y
por -2 em 64 bits e limita a saída horizontal a valores pares entre -128 e
+126. Assim cada envio consome exatamente `source Y = -report.x / 2`.
Usar +127 aqui perderia meio passo original por divisão inteira.
O eixo vertical consome `source X = report.y`. Excedentes permanecem pendentes
para o próximo pacote. USB ocupado não consome o movimento nem reaplica ganho
a dados já transformados. A assinatura do construtor permanece a da base.

## Evidências e teste físico

Os 13 testes host passaram em Release com assertions ativas. A suíte de
movimento verifica 32.385 vetores pequenos, uma trajetória circular convertida
em elipse ponto a ponto, limites de 32 bits, e 1.156 combinações de deltas nos
quatro perfis (incluindo limites dos pacotes e os extremos de 16 bits).
Confere deslocamento total dobrado, wheel/pan e conteúdo idêntico ao repetir
a preparação de um pacote ainda não enviado.

CI desta branch compila o firmware Pico 2 W com o toolchain/SDK fixado pelo
projeto. O teste físico desta variante ainda depende do usuário:

1. Desative o preset de rotação/ganho do Input Remapper para não duplicar o efeito.
2. Grave o novo UF2 pelo BOOTSEL e reconecte/pareie o mouse normalmente.
3. Percorra a mesma distância física para frente e para o lado: o primeiro
   movimento deve deslocar o ponteiro horizontalmente duas vezes mais que o
   deslocamento vertical produzido pelo segundo.
4. Desenhe círculos com a mão: o ponteiro deve desenhar elipses deitadas.
5. Confira movimentos pequenos, diagonais, movimentos rápidos e arraste.
6. Confira botões e scroll, troque perfis e reinicie o Pico; o ganho deve permanecer.

Para retornar à rotação sem ganho extra, use o UF2 anterior `a8c4c1d`.
