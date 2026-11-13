# Dependências

- JUCE **8.0.12**, commit `29396c22c93392d6738e021b83196283d6e4d850`:
  https://github.com/juce-framework/JUCE/tree/8.0.12 . AGPLv3 ou licença comercial
  JUCE aplicável. A licença MIT do nosso código não relicencia a dependência.
  Para distribuição fechada, obtenha a licença adequada. Fontes próprias completas
  acompanham a distribuição pública no repositório e no ZIP Sources.
- VST3 SDK incorporado ao JUCE, em
  `modules/juce_audio_processors_headless/format_types/VST3_SDK`.
  Seus avisos/licenças são incluídos nos ZIPs; preserve-os na redistribuição.
- xaymar-vst2: ABI independente, copyright Michael Fabian 'Xaymar' Dirks,
  DISTRHO/DPF commit `4238e1c7f0351bbe488d79f0899c540543ac7583`.
  Header/licença BSD em `third_party/xaymar-vst2`; não é o SDK proprietário VST2
  e não concede direitos de marca.
- Header da API REAPER, Cockos Incorporated, SDK commit
  `b2ca1ec92c9a7864fb72346037986a384971ed03`. Licença permissiva integral
  preservada em `third_party/reaper-sdk/reaper_plugin.h`. Usado só na extensão
  de teste Windows. O REAPER é baixado do fornecedor no CI e não redistribuído.

Visual Studio/MSVC são ferramentas da Microsoft. A partir de 0.2.1, as compilações Windows usam
runtime estático /MT em Release; estes módulos não requerem VC++ Redistributable.
Os termos de redistribuição do runtime Microsoft continuam aplicáveis.
Nenhum código, logotipo ou recurso gráfico da Waves foi incluído.
