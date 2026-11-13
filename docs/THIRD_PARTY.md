# Dependências e fontes

- JUCE 8.0.4 (build padrão): https://github.com/juce-framework/JUCE/tree/51d11a2be6d5c97ccf12b4e5e827006e19f0555a
  Licença AGPLv3 ou licença comercial JUCE. Os arquivos de licença e avisos estão
  no checkout da dependência e se aplicam ao software combinado.
- JUCE 7.0.12 (compilação cruzada MinGW): https://github.com/juce-framework/JUCE/tree/4f43011b96eb0636104cb3e433894cda98243626
  Licença GPLv3 ou licença JUCE 7 aplicável. Não confundir seus termos com JUCE 8.
- VST3 SDK: versão incorporada em cada versão JUCE, em
  `modules/juce_audio_processors/format_types/VST3_SDK`; consultar os respectivos
  arquivos de licença. Não há SDK VST2 no checkout.
- MinGW-w64/GCC/binutils e pacotes Debian: ferramentas de compilação baixadas dos
  repositórios Debian assinados; a toolchain não integra o código-fonte do plugin.

O código próprio sob MIT não relicencia JUCE ou outras dependências. Para distribuir
os binários, cumpra a licença escolhida para o conjunto e preserve os avisos
aplicáveis. A distribuição de código fechado requer avaliar a licença comercial
adequada. Não se presume que uma licença de JUCE conceda direitos sobre VST2.

- xaymar-vst2: interface independente para interoperabilidade, copyright Michael
  Fabian 'Xaymar' Dirks, proveniente de DISTRHO/DPF commit
  `4238e1c7f0351bbe488d79f0899c540543ac7583`. O header e os avisos originais estão
  em `third_party/xaymar-vst2`. A distribuição inclui o LICENSE BSD com suas
  condições; não é o SDK da Steinberg nem concede direitos de marca.
