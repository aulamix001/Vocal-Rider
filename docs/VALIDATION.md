# Validação nesta máquina

## Linux x86_64 / GCC 14 / JUCE 8.0.4

- Build Release de VST3 e standalone: aprovado.
- CTest: 4 testes executados, 4 aprovados, 0 falhas, 0 ignorados.
- DSP: 12 combinações de 44.1/48/96 kHz e blocos de 1/64/511/1024 amostras.
  Verifica silêncio, resposta a mudanças de nível, limites, estéreo vinculado,
  retorno a Idle, convergência ao Target, bypass, entrada NaN/Inf, override manual,
  mudanças abruptas e influência musical. Aborta se houver `new` no callback DSP.
- Mesmo teste DSP com AddressSanitizer e UndefinedBehaviorSanitizer: aprovado.
- Plugin: parâmetros/estado, Read, Write, Off, mono/estéreo, habilitação de sidechain,
  criação e renderização do editor, Undo/Redo, A/B e Fast/Slow: aprovado.
- VST3 real: `dlopen`, fábrica, inicialização, ativação e 500 blocos estéreo
  processados com saída finita e não silenciosa: aprovado.
- Captura do editor: `build/interface.png`, 420×740, inspecionada visualmente.
- Standalone: iniciou sob Xvfb e permaneceu vivo durante os 4 segundos do teste;
  término por timeout foi intencional. Sem dispositivo ALSA/MIDI na nuvem:
  reprodução/escuta de áudio ao vivo não validada.

## Limites de evidência

O teste de Write verifica publicação do parâmetro; não demonstra gravação numa
lane de automação de um DAW. Read não valida edição de envelopes em um host real.
O teste de VST3 é um host mínimo e não substitui testes em Reaper, Cubase ou outro
DAW. Nenhum teste demonstra equivalência sonora ao algoritmo proprietário.

Build Windows x64 VST3/standalone via MinGW com JUCE 7.0.12: compilação concluída.
Build Windows com MSVC: workflow preparado, não executado nesta tarefa. A tentativa
de cross-build com JUCE 8/MinGW confirmou suporte removido. A alternativa com
JUCE 7.0.12 compilou VST3 e standalone Windows x64. O binário PE32+ foi inspecionado:
exporta GetPluginFactory/InitDll/ExitDll e depende apenas de bibliotecas Windows,
sem DLLs MinGW externas.
Carregamento, interface e áudio em Windows não foram executados nesta máquina.
O adaptador independente VST2 foi compilado para Windows x64/JUCE 7 e Linux
x64/JUCE 8, sem SDK Steinberg. O quarto teste verifica o módulo real: fábrica,
ABI, parâmetros, áudio sem alocações C++ no callback, bypass, chunks de estado,
Read/Write, sidechain e embedding da interface numa janela X11. O ambiente
virtual usa Xvfb com o gerenciador xfwm4; sem um WM, JUCE falhou com BadAtom,
resolvido iniciando uma sessão gráfica de teste completa. A DLL Windows exporta
VSTPluginMain e não depende de DLLs adicionais MinGW. Não foi executada num DAW
Windows. Veja docs/VST2.md.
Seleção interativa de arquivos nos diálogos Load/Save não foi automatizada.
A/B e Undo/Redo foram exercitados nos próprios controles da interface.

## Investigação posterior do Windows 11 / 0xC0E90002

O usuário confirmou Controle Inteligente de Aplicativos ativado. O código oficial
é STATUS_SYSTEM_INTEGRITY_POLICY_VIOLATION. Ambos os módulos são sem assinatura.
Foram acrescentados hosts de teste Windows reais: os módulos publicados passaram
em LoadLibrary, fábrica/processamento VST3 e áudio/estado/automação/editor VST2,
executados em Wine 10. Isso não reproduz a política de segurança do Windows 11.
O fluxo de assinatura confiável está preparado, mas não foi executado por ausência
de certificado/chave; os arquivos de download permanecem sem assinatura.
