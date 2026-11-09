# Recuperação ZMK após RMK — nRF52840 clone nice!nano V2

Esta branch isolada serve para testar ZMK em um nRF52840 clone nice!nano V2 que ainda entra no bootloader UF2, mas deixou de iniciar ZMK depois de receber RMK. A branch `master` e o PR `gbrtzt-runtime-v1` permanecem sem alterações.

## Por que isso ocorre?

O RMK pode utilizar a região da flash originalmente ocupada pelo SoftDevice, enquanto uma compilação ZMK comum normalmente a reserva. O snippet oficial `nrf52840-nosd` utiliza o mapa de memória sem reservar o SoftDevice. Consulte:

- https://rmk.rs/main/docs/getting_started/faq#zmk-no-longer-works-after-flashing-rmk
- https://zmk.dev/docs/config/system#nrf52840-nosd

**Aviso:** o ZMK informa que o uso desse snippet pode apagar o SoftDevice, e, nesse caso, firmwares construídos sem o snippet não funcionarão até a restauração do SoftDevice. Use somente no controlador específico que já passou pelo RMK e precisa desse layout. Não faça testes cruzados em controladores que ainda executam a configuração original.

## Compilação no GitHub

1. Abra https://github.com/gbrlttcanal-oss/corne-oled-zmk/actions/workflows/build.yml
2. Clique em **Run workflow**.
3. Em **Use workflow from**, selecione `recovery/zmk-after-rmk-nosd`.
4. Clique em **Run workflow** novamente.
5. Após a conclusão com sucesso, abra a execução e baixe o artifact `firmware`.
6. Extraia o ZIP. Os nomes esperados são:
   - `corne-left-nosd-studio.uf2`: controladora esquerda, ZMK Studio via USB.
   - `corne-right-nosd.uf2`: controladora direita.
   - `settings-reset-nosd.uf2`: limpeza opcional de configurações persistentes.

O GitHub Actions compila automaticamente quando há commits nesta branch. Você também pode usar **Run workflow** manualmente.

## Teste com o controlador solto

1. Mantenha outras metades de teclado desligadas.
2. Conecte o controlador por USB de dados ao PC.
3. Entre no bootloader com duplo reset.
4. Copie `corne-left-nosd-studio.uf2` para a unidade UF2 e aguarde a reinicialização.
5. Abra https://zmk.studio/ no Chrome/Edge e teste a conexão USB.

**Aparecer no ZMK Studio exige que a aplicação ZMK esteja executando e que a interface USB RPC esteja acessível.** A unidade de bootloader não é o próprio ZMK Studio.

## Se não funcionar

- Confirme que compilou a branch correta e copiou o arquivo **left** com `nosd`.
- Teste outro cabo USB de dados e veja o Gerenciador de Dispositivos do Windows.
- Se o ZMK iniciar mas o Bluetooth não anunciar/conectar, pode ser necessário gravar `settings-reset-nosd.uf2`, aguardar, e depois gravar novamente `corne-left-nosd-studio.uf2`. **Settings reset apaga pareamentos e outras configurações persistentes**; não é o primeiro passo.
- Anote o nome da unidade UF2, o conteúdo de `INFO_UF2.TXT` e qualquer erro do GitHub Actions.

Esta branch não incorpora a extensão experimental GbrTzT Runtime (macros/tap dance/RGB). Primeiro confirme o funcionamento do ZMK base e do Studio; depois aplique `nrf52840-nosd` à branch experimental, se necessário.
