# Vocal Balance

Plugin independente de ajuste automático de nível vocal em C++17/JUCE 8.0.12.
VST3, módulo de compatibilidade VST2 e standalone; Windows e Linux x64.
**Implementação própria baseada nas páginas e imagens recebidas.** Veja
[decisões e limitações](docs/REFERENCES.md). Não há alegação de equivalência ao Waves Vocal Rider.

## Download compilado — Windows x64

[**Baixar VST2 Windows x64 — MSVC 0.2.0**](https://github.com/aulamix001/Vocal-Rider/raw/refs/heads/main/downloads/VocalBalance-0.2.0-VST2-Windows-x64-MSVC.zip)

[**Baixar VST3 Windows x64 — MSVC 0.2.0**](https://github.com/aulamix001/Vocal-Rider/raw/refs/heads/main/downloads/VocalBalance-0.2.0-VST3-Windows-x64-MSVC.zip)

Versão **0.2.0**, compilada no Windows com **Visual C++ / JUCE 8.0.12**, com **4/4 testes nativos aprovados** e VST2/VST3 encontrados e carregados no **REAPER 7.85/x64**.

VST2: copie a DLL para a pasta VST2 do DAW. VST3: copie a pasta `.vst3` inteira para `C:\Program Files\Common Files\VST3\`. Remova as cópias antigas antes de atualizar a busca.

**Sem assinatura Authenticode:** o carregamento com Controle Inteligente de Aplicativos ativado ainda precisa ser confirmado no seu computador. [Comparação e diagnóstico](docs/WINDOWS_LOADING.md).

[Informações do build](downloads/WINDOWS_NATIVE_BUILD.json) · [Checksums](downloads/SHA256SUMS.txt)

## Compilar no Windows

Instale Visual Studio 2022 com Desktop development with C++, CMake e Git.
Você pode abrir esta pasta no Visual Studio e selecionar o preset `windows-vs2022-x64`.
No Developer PowerShell, a partir da raiz do checkout:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

Release usa CRT estático `/MT` (Debug `/MTd`); o projeto recusa Windows não x64.
Os exports e imports dos binários são auditados antes da publicação.

O primeiro configure baixa o JUCE oficial fixado por commit. Para uma cópia local,
acrescente `-DJUCE_SOURCE_DIR=C:/caminho/JUCE`. Os artefatos ficam em
`build/VocalBalance_artefacts/Release/`. Copie a pasta `VocalBalance.vst3` para
`C:\Program Files\Common Files\VST3` e faça o DAW procurar plugins novamente.
O executável standalone fica em `Standalone`. A compilação Windows precisa ser
validada em Windows; sucesso no Linux não a comprova.

## Compilar no Linux

Debian/Ubuntu, com privilégios administrativos somente para pacotes do sistema:

```sh
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build git pkg-config \
  libasound2-dev libx11-dev libxext-dev libxinerama-dev libxrandr-dev \
  libxcursor-dev libfreetype-dev libfontconfig-dev libgl1-mesa-dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

Copie o bundle VST3 de `build/VocalBalance_artefacts/Release/VST3/` para `~/.vst3/`.
Execute `build/VocalBalance_artefacts/Release/Standalone/VocalBalance` numa sessão
gráfica e configure o dispositivo de áudio pelo botão de opções do JUCE.
Sem servidor gráfico ou dispositivo, use os testes offline. Para testar somente o
DSP, sem baixar JUCE: `cmake -S . -B build-dsp -DVB_BUILD_PLUGIN=OFF`, seguido de
`cmake --build build-dsp` e `ctest --test-dir build-dsp --output-on-failure`.

## Operação

Insira na trilha de voz. Ajuste Target conforme o nível RMS desejado, Vocal Sensitivity para distinguir voz de ruído, e os limites de ganho. Attack Slow produz
movimento mais lento; Fast acompanha mudanças mais rapidamente. Idle define o
ganho nos intervalos sem voz. Envie o instrumental ao sidechain para usar Music
Sensitivity. Ajuste Output para o nível final. Bypass mantém áudio seco.
Todos os controles são parâmetros do host; o projeto do DAW salva o estado.
Off usa o detector interno; Write publica o fader Rider ao DAW a 30 Hz; Read segue
a automação do parâmetro Rider. Arme Write/Touch no DAW para gravar. A descoberta no REAPER Windows faz parte da publicação 0.2.0; Studio One ainda requer validação.
Os presets de fábrica substituem Target, Range, Vocal Sensitivity, Attack e Output,
incluindo Music e Idle, preservando o modo de automação. Valores manuais são salvos no estado do host. Load/Save importam e exportam
arquivos XML `.vbpreset`; A/B compara duas configurações da sessão e as setas
desfazem/refazem alterações de parâmetros. O botão ? abre a ajuda. O ganho pode causar clipping em etapas posteriores.

## Licenças

Código novo: MIT, veja LICENSE. JUCE é dependência externa, não incluída na licença
MIT deste projeto. JUCE 8 oferece AGPLv3 ou licença comercial da JUCE; distribuir
um produto fechado exige avaliar e obter a licença adequada. Veja os termos em
https://juce.com/legal/juce-8-licence/ e os arquivos da versão fixada. As licenças
terceiras incluídas no JUCE e no SDK VST3 também devem ser preservadas ao distribuir.
Nenhum logotipo, código ou recurso visual da Waves foi incluído.

## Compatibilidade VST2 (Windows/Linux x64)

O build padrão gera também `VocalBalance_artefacts/Release/VST2/Vocal Balance.dll`
(Windows) ou `Vocal Balance.so` (Linux). É um adaptador independente baseado na
interface aberta xaymar-vst2/BSD vendorizada em `third_party/`, sem o SDK legado
Steinberg. Compartilha o DSP, a interface, presets e parâmetros do plugin principal.
Veja [detalhes de build e validação](docs/VST2.md).

Copie a DLL para a pasta de plugins VST2 configurada no seu DAW e faça nova busca.
O módulo tem quatro entradas: voz L/R nos canais 1/2 e música L/R nos canais 3/4,
e duas saídas estéreo. Deixe as entradas de música sem sinal quando não utilizar
sidechain. O roteamento depende dos recursos do host. Só há build de 64 bits.

Para compilar apenas esse módulo:

```sh
cmake --build build --config Release --target VocalBalance_VST2 --parallel 2
```

Use `-DVB_BUILD_VST2_COMPAT=OFF` para desativá-lo. A integração anterior com um
SDK legado obtido legalmente continua opcional em `VB_VST2_SDK_PATH`; quando esse
caminho é fornecido, usa-se o wrapper JUCE do SDK em vez do adaptador independente.

O workflow `.github/workflows/build.yml` prepara os artefatos Windows quando
executado no GitHub. Criar o workflow não executa nem valida um DAW Windows.

Veja a [arquitetura da reconstrução 0.2.0](docs/REBUILD.md).
