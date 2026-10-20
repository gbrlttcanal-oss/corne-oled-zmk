# GbrTzT runtime v1 — firmware experimental

Esta extensão permite alterar macros e tap dances pela conexão USB do GbrTzT-Studio v0.5 e persistir os dados na memória do controlador. Depois de instalar este firmware uma vez, editar o conteúdo dos oito slots não exige recompilar nem flashar novamente.

## Controlador de teste

- Alvo: Corne, `nice_nano_v2`, incluindo o clone nRF52840 que o usuário já validou com o firmware original.
- Controlador solto: usar **gbrtzt-runtime-v1-corne-left.uf2**. O lado esquerdo é a central com USB Studio. O arquivo direito é para a outra metade do teclado completo.
- OLED mantido, RGB desabilitado no pacote principal. Três camadas definidas e duas reservadas; o Studio permite habilitar as reservadas até chegar a cinco.
- Não usar `settings_reset` para uma atualização normal: ele apaga configurações e pareamentos. Não usar os arquivos `compile-check-only-rgb` no controlador solto.
- Base fixada: `cormoran/zmk@4493783ef88ce2e653bf8217c92ee17140df71e3`, derivada de ZMK v0.3, com transporte RPC customizado. Não é o ZMK oficial sem alterações. O protocolo padrão de remapeamento permanece disponível.
- A branch `gbrtzt-runtime-v1` é separada da `master`; a versão original permanece disponível para retorno.

## Primeiro teste

1. Anote o mapa atual e mantenha uma cópia do UF2 original. A compatibilidade das configurações salvas entre as duas bases ainda precisa de teste físico.
2. Feche outros configuradores, coloque o controlador no bootloader pelo procedimento que já usa e copie o UF2 **left** para a unidade USB do bootloader. Aguarde reiniciar.
3. Abra https://gbrtzt-zmk-studio.ivorysun1024.chatgpt.site/device.html no Chrome/Edge do computador. Clique em conectar e escolha a porta.
4. Confira o mapa lido. Em **Macros**, escolha M1, selecione o modo texto e escreva `Bom dia!`. Escolha ABNT2 ou US conforme o layout ativo no sistema operacional e clique em **Salvar macro**.
5. Selecione uma posição do teclado, clique em atribuir a macro e grave a alteração do mapa. Salvar o slot e atribuí-lo a uma tecla são duas operações distintas.
6. Desconecte o cabo, reconecte, leia M1 e confirme que a frase permanece. Confira também a atribuição no mapa.
7. Em **Tap dance**, salve T1 com Esc no primeiro toque e Tab no segundo. Atribua a outra posição e grave o mapa. Reconecte e confirme os dados novamente.

O controlador solto permite verificar comunicação, gravação e releitura. A digitação da frase e a contagem de toques precisam de eventos físicos da matriz; o editor não envia um comando remoto de execução de macro. Faça essa parte quando houver teclas conectadas e usando um editor de texto vazio.

## Limites e comportamento

- 8 macros persistentes; 128 passos por macro, após expandir o texto em teclas. Uma macro executa por vez; outra solicitação durante a execução é recusada.
- Texto de até 255 bytes UTF-8, limitado também aos 128 passos. ABNT2 inclui acentos portugueses por teclas mortas; US aceita os caracteres do layout US. Emojis e Unicode arbitrário não são suportados. A configuração do sistema precisa coincidir com a escolhida no app.
- Nome de até 31 bytes UTF-8. Tempos de 0–5000 ms por passo e duração programada máxima de 30 segundos. Toques têm mínimo de 10 ms pressionados mais 10 ms entre eventos; outros passos têm mínimo de 1 ms.
- Passos: tocar, pressionar, soltar e esperar. Até 16 teclas explicitamente mantidas. A validação exige soltar todas. Alterar um slot em execução não muda a sequência em andamento.
- 8 tap dances; até 4 ações, cada uma sendo uma tecla com modificadores ou uma referência a um slot de macro. Intervalo de 100–1000 ms desde o último pressionamento. Decide também ao atingir o número máximo de toques ou pressionar outra posição. Uma ação de tecla decidida enquanto a tecla física está mantida só é liberada quando ela é solta. Até quatro dances ativos simultâneos.
- Apagar o conteúdo de um slot não remove suas atribuições no mapa. Elas ficam sem ação até preencher o slot novamente. Um dance que referencia uma macro vazia não produz texto.
- Leituras não gravam. Cada escrita valida tamanho e conteúdo, exige a revisão esperada, salva em NVS e só então atualiza a cópia em RAM. O app relê e compara os bytes após salvar. Desconectar durante a gravação exige releitura antes de tentar novamente.
- A demo é independente: seu JSON não é importável no firmware e seus limites são diferentes.

## RGB opcional

A aba oferece ligar/desligar, cor e brilho de iluminação uniforme quando o firmware anuncia RGB. Os arquivos entregues para o controlador solto anunciam RGB indisponível.

Quando o teclado tiver LEDs, será preciso definir modelo, quantidade, pino de dados e alimentação adequados à montagem e então habilitar `CONFIG_ZMK_RGB_UNDERGLOW=y` e `CONFIG_WS2812_STRIP=y` com o devicetree correspondente. Não basta habilitar um pino arbitrário. O CI compila as duas metades com os padrões do shield Corne apenas para verificar o código; isso não valida a fiação do teclado futuro.

A extensão grava cor/brilho, seleciona o efeito uniforme e aplica o estado via comportamento global RGB. A central reaplica o valor ao iniciar e quando a metade periférica reconecta. A tela relê o último valor salvo pela extensão, não acompanha alterações feitas por outras teclas RGB. Não há configuração individual de LEDs ou animações nesta versão.

## Validação

Testes de host compilam `format.c` e o próprio `runtime.c`, com adaptadores determinísticos de timer, flash e eventos HID: limites, revisão concorrente, erro de flash, restauração, CRC inválido, snapshots, liberação de teclas, toque simples/duplo, tecla mantida, interrupção por outra posição, macro em tap dance e reset. Sanitizers de endereço e comportamento indefinido habilitados no CI.

O workflow compila esquerda/direita com RGB desligado e, em uma etapa separada, com RGB ligado. Os testes do aplicativo usam protobuf e porta serial simulada. Esses testes e compilação não substituem validação física de USB, alimentação, matriz, Bluetooth split, OLED, layout do sistema ou LEDs. Esta primeira versão aguarda o teste do usuário.

## Protocolo para manutenção

Subsistema seguro `gbrtzt__runtime_v1`, descoberto dinamicamente pelo RPC custom. Versão 1; inteiros little endian. Requisição: versão/operação/tipo/slot (um byte cada), revisão esperada u32 e payload. Resposta: mesmo prefixo, status errno positivo u16, revisão u32 e payload. Operações: 0 capacidades, 1 ler, 2 gravar. Tipos: 1 macro, 2 dance, 3 RGB. Consulte `format.h`, `format.c` e `rpc.c` para tamanhos e campos; app correspondente em `dist/runtime-model.js`.

Registros NVS: cabeçalho `GRT1`, versão/tipo/slot/reservado, revisão u32, CRC32 do payload; dados a partir do byte 16. Mudanças de formato exigem nova versão e migração explícita. O serviço respeita o bloqueio Studio; este firmware mantém `CONFIG_ZMK_STUDIO_LOCKING=n`, conforme a configuração original do usuário.
