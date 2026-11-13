# Requisitos e correspondência com o manual

Referências recebidas no chat: imagem comparativa Vocal Rider/Vocal Rider Live;
introdução 1.1–1.2; conceitos/componentes/toolbar 1.3–1.5; quickstart capítulo 2;
interfaces 3.1; controles 3.2, páginas impressas 7–11. Foram lidos os trechos
fornecidos, não um PDF completo. Não foram copiados logotipos ou bitmaps do produto.

| Documentado nas imagens | Implementação |
|---|---|
| Target −42 a 0 dBFS, padrão −21 | Parâmetro e slider horizontal, indicador do nível vocal |
| Vocal Sensitivity ±12 dB, padrão 0 | Sensibilidade controla limiar do detector |
| Activity Ride/Idle | Indicador acompanha detector |
| Attack Fast/Slow, padrão Slow | 100/600 ms de correção, 10/50 ms de detecção: tempos são inferidos |
| Music Sensitivity ±12 dB, padrão 0 | Ajusta intensidade de influência do sidechain |
| Music LED | Acende quando há sinal no sidechain; brilho proporcional ainda não implementado |
| Range min −12 a +11, padrão −6 | Alça inferior e parâmetro automatizável |
| Range max −11 a +12, padrão +6 | Alça superior e parâmetro automatizável |
| Idle ±12 dB, padrão 0 | Retorno suave para Idle, limitado pelo Range |
| Effective Range 1–24 dB, padrão 12 | Leitura numérica; arrasto mantém separação mínima de 1 dB |
| Rider ±12 dB, automático ou manual enquanto tocado | Fader real, override por arrasto, retorno automático ao soltar |
| Off / Write / Read | Off ignora Rider do host; Write notifica host a 30 Hz; Read usa Rider do host |
| Output ±12 dB, padrão 0; Clip LED | Trim suavizado e indicador de clipping |
| Mono/stereo e sidechain de acompanhamento | Buses mono/stereo e sidechain estéreo opcional |
| Zero latency, sem pré-análise | Processamento causal, nenhuma latência declarada |
| Live: Spill e ajuste a ruído de palco | Variante Live não implementada nesta versão |
| Toolbar Waves: presets, A/B, undo/redo | Presets próprios, Undo/Redo, A/B, Load/Save e ajuda; sem recursos Waves |

## Algoritmo próprio e diferenças

O manual descreve comportamento, mas não fornece o algoritmo. Esta implementação
usa detector RMS estéreo vinculado, diferença em dB entre Target e nível detectado,
limites de ganho e suavização exponencial. O detector de atividade é um limiar
ajustável; não classifica fonemas, respiração, ruído e voz como um detector
proprietário poderia fazer. A relação musical usa nível RMS de referência de
−18 dBFS e deslocamento limitado a ±12 dB; referência e equação são inferências,
não parâmetros extraídos do manual. Sem música, o ajuste depende apenas da voz.

Off/Write/Read estão implementados via parâmetro Rider do host. Write publica
movimentos a 30 Hz na thread de mensagens, sem operações do host na thread de
áudio. É preciso armar Write/Touch no DAW e validar compatibilidade no host real.
Não é automação por amostra. Read suaviza a leitura em 10 ms. O mouse assume o
controle enquanto arrasta. Suporte a superfície física/touch não foi validado.

A interface de 420×740 usa proporções verticais, metal grafite escovado no topo,
alumínio prateado, reflexos verticais e couro granulado no rodapé. Knobs chanfrados,
faders com ranhuras e sombras, LEDs com grade e brilho e botões de automação com
relevo são desenhados por código. A barra superior contém Undo/Redo, presets,
comparação A/B, Load/Save e ajuda. É desenho original aproximado,
não cópia pixel a pixel; controles auxiliares e tipografia diferem. As imagens
não fornecem coordenadas/dimensões originais com precisão suficiente para afirmar
identidade. Os presets Default/Gentle/Firm, bypass e marca VB/Vocal Balance são próprios.
A/B mantém dois estados durante a vida do editor. Load/Save usa arquivos XML
`.vbpreset`; o estado do projeto continua sendo salvo pelo host.

Sem limiter: trim/ganho podem elevar picos acima de 0 dBFS. Não há lookahead,
compressão com curva de razão ou alteração espectral intencional. As garantias de
real-time cobrem nosso DSP, que não aloca memória, bloqueia ou acessa disco.
