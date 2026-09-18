# blu2usb-picow

Firmware plug-and-play para Raspberry Pi Pico 2 W que recebe dispositivos HID Bluetooth e expõe uma identidade USB HID estável ao host, sem software no sistema operacional.

## Estado do projeto

**G07 reconstruído a partir do G06 aceito:** branch
`gate/g07-validated-classic-from-g06`. A solução GT T1/BKB-3G foi validada
fisicamente na POC; a validação física da integração G07 continua pendente.
Leia primeiro o [registro definitivo da solução, origem e teste](docs/technical/06-g07-keyboard-solution.md)
e o [manifesto de proveniência](docs/reference/g07-keyboard-provenance.json).
Nunca restaure a abertura HID síncrona dentro do evento de bonding.

Este repositório é uma reconstrução limpa e madura do trabalho experimental anterior. O código antigo não é base estrutural: somente evidências físicas, decisões já aceitas e soluções comprovadas podem ser reexpressas sob os contratos deste repositório.

A implementação começa somente depois do **BLU2USB-G00 — Contract Freeze**.

## Fontes normativas

A precedência documental é:

1. `docs/ux/01-screen-layouts.md` — layouts literais e regras específicas por tela;
2. `docs/product/00-product-contract.md` — comportamento de produto e perfis;
3. `docs/ux/00-interaction-visual-contract.md` — interação, navegação e cores;
4. `docs/technical/00-architecture-contract.md` — fronteiras técnicas;
5. `plan/gates.md` — ordem e critérios de implementação;
6. `docs/reference/00-reuse-evidence.md` — conhecimento reaproveitável, nunca autoridade sobre os contratos acima.

Em caso de conflito, não se interpreta silenciosamente: a implementação para e o contrato é corrigido antes do código.

## Regra de layout

Os layouts documentados são congelados. Texto literal não pode ser reformulado por conveniência. Somente conteúdo explicitamente marcado como exemplo/dinâmico pode ser proposto pela implementação, respeitando a grade 9x21, as regiões visuais e as regras semânticas de cor.

## Regra de depuração

O firmware deste repositório **não terá ferramentas de depuração embarcadas**: sem USB CDC de diagnóstico, UART de diagnóstico, descriptor USB alternativo de debug, tela de debug ou firmware paralelo de debug.

Diagnóstico deve ocorrer por testes host/CI, revisão de estado, critérios físicos no LCD/HAT e comportamento observável pelo sistema operacional.

## Gates

O planejamento congelado está em `plan/gates.md` e o checklist contínuo em `plan/implementation-checklist.md`.
