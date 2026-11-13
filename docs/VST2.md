# Módulo independente compatível com VST2

O módulo reaproveita `VocalBalance` e `BalanceEditor`, com um adaptador de protocolo
próprio em `Source/Vst2Adapter.cpp`. A estrutura binária é definida pelo header
aberto xaymar-vst2, BSD, obtido do projeto DISTRHO/DPF em commit fixo. Não foi
utilizado ou baixado o SDK VST2 proprietário. A origem e os avisos estão em
`third_party/xaymar-vst2/`. Os termos JUCE continuam aplicáveis ao binário completo.

## Artefatos

- Windows x64: `build-win7/VocalBalance_artefacts/Release/VST2/Vocal Balance.dll`.
- Linux x64: `build/VocalBalance_artefacts/Release/VST2/Vocal Balance.so`.

O Windows foi compilado com JUCE 7.0.12/MinGW-w64; o Linux com JUCE 8.0.4/GCC 14.
Ambos exportam `VSTPluginMain`. A DLL foi inspecionada como PE32+ x64 e não depende
de runtimes MinGW externos. O build não inclui uma versão de 32 bits.

## Instalação e operação

Copie a DLL para a pasta VST2 escolhida no DAW, por exemplo
`C:\Program Files\VstPlugins\`, e atualize a busca de plugins. Se o host só
aceita VST3, use o pacote VST3 já fornecido. No Linux, configure a busca para a
pasta que contém o `.so` (frequentemente `~/.vst/`).

O adaptador apresenta 4 entradas e 2 saídas. Canais 1/2: voz estéreo; 3/4: música
para sidechain; saídas 1/2: voz processada. O host precisa fornecer silêncio nas
entradas não roteadas. A entrada musical não é misturada na saída. Para voz mono,
o host pode alimentar o par principal com o mesmo sinal, conforme seu roteamento.

Os 11 parâmetros são automatizáveis. Off/Write/Read e o fader mantêm os controles
da versão VST3; Write comunica o movimento pelo callback de automação do host.
Arme Write/Touch no DAW para gravar. Estado é exportado/importado em chunks. Os
presets do host e os arquivos `.vbpreset` usam o mesmo processador da aplicação.

## Reproduzir o build

Linux, após os passos normais do README:

```sh
cmake --build build --target VocalBalance_VST2 vst2_smoke --parallel 2
DISPLAY=:88 ctest --test-dir build -R vst2_module --output-on-failure
```

Para testar embedding da interface, o display precisa de X11 e um gerenciador de
janelas. Veja `docs/CLOUD.md`. Windows com Visual Studio usa o mesmo alvo e
`--config Release`. O cross-build desta instância usa os comandos de CLOUD.md e:

```sh
cmake --build build-win7 --target VocalBalance_VST2 --parallel 2
```

O adaptador fica ativo por padrão. `VB_BUILD_VST2_COMPAT=OFF` o desativa. O caminho
opcional de SDK legado mantém suporte ao wrapper original JUCE, para quem já
possui tal SDK; esse caminho não foi usado nos artefatos entregues.

## Evidência e limites

O host mínimo de teste carrega o módulo real e verifica os metadados binários,
parâmetros e índices inválidos, áudio estéreo, blocos maiores que o tamanho
negociado, ausência de `new` durante processamento, bypass, chunks de estado,
sidechain, Read, callbacks Write e abertura/fechamento da interface numa janela
nativa. A suíte completa terminou com 4/4 testes aprovados.

Execução em um DAW Windows e compatibilidade com diferentes hosts ainda precisam
de validação. A interface binária não constitui certificação ou endosso da
Steinberg. Para áudio são suportados float32 e layout estéreo fixo com sidechain;
processamento float64 e configuração dinâmica de layouts não são anunciados.
No Linux, o host deve enviar as chamadas de idle do editor para processar eventos
da interface. No Windows, a janela usa o loop nativo do host.
