# Vocal Balance 0.2.0 — reconstrução

Nova estrutura C++17/JUCE 8.0.12, gerada por CMake para Visual Studio 2022 x64
(v143), VST3 oficial JUCE, standalone e entrada VST2 de interoperabilidade.
As versões antigas permanecem no histórico Git (tag `v0.1.1-before-rebuild`).

- `Source/Dsp/GainRider.h`: detector e ganho, sem JUCE, alocações, disco ou locks.
- `Source/PluginProcessor.*`: buses, parâmetros APVTS e estado schema 2.
- `Source/PluginEditor.*`, `Source/UI/*`: controles reais e desenho próprio.
- `Source/LegacyVst2.cpp`: ABI VST2 independente; não é wrapper do VST3.
- `tests/*`: sinais sintéticos, persistência, protocolos dos módulos e descoberta
  em REAPER Windows através de uma extensão de teste (não distribuída no plugin).

A identificação `VB20` e o bundle `audio.independent.vocalbalance2` são novos.
O nome exibido é Vocal Balance; arquivos são `VocalBalance.dll` e
`VocalBalance.vst3`. Projetos/presets da versão 0.1 não são migrados automaticamente.
Uma nova identificação evita reutilizar resultados antigos de varredura;
não altera políticas de segurança do Windows.

O algoritmo é próprio: RMS causal, histerese de atividade e repouso, curvas
exponenciais de movimento e trim. Fast: detector 10/80 ms, ganho 100 ms; Slow:
40/200 ms, ganho 500 ms. Retenção de atividade: 120 ms; manual: 10 ms; output e
bypass: 15 ms. Esses tempos e a fórmula de sidechain são inferidos, não valores
confirmados do algoritmo proprietário. Music compara RMS à referência -18 dBFS,
com influência proporcional à sensibilidade e deslocamento limitado a ±12 dB.
Não usa lookahead e reporta latência zero. Não é um limitador de picos.

A publicação automática exige quatro testes nativos aprovados, PE x64 produzido
pelo linker Microsoft e descoberta/instanciação dos dois formatos no REAPER real.
O relatório publicado registra a versão do host. Isso não valida Studio One,
Smart App Control nem a instalação específica no computador do usuário.
