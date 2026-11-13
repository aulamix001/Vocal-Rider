# Ambiente desta tarefa

O ambiente não oferece sudo nem instalação global de pacotes. Ferramentas e headers
foram preparados em `/workspace/toolchain`, fora do checkout. Os pacotes Debian
foram baixados via APT com índices assinados e extraídos com `dpkg-deb`; não foi
desativada verificação de TLS ou de assinatura. O repositório não inclui esses caches.

Para repetir um build nesta instância:

```sh
source /workspace/toolchain/activate.sh
export CMAKE_BUILD_PARALLEL_LEVEL=2
cmake -S /workspace/Vocal-Rider -B /workspace/Vocal-Rider/build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DJUCE_SOURCE_DIR=/workspace/toolchain/JUCE \
  -DCMAKE_PREFIX_PATH=/workspace/toolchain/sysroot/usr
cmake --build /workspace/Vocal-Rider/build --parallel 3
DISPLAY=:88 ctest --test-dir /workspace/Vocal-Rider/build --output-on-failure
```

O display :88 foi criado com Xvfb para validação gráfica. Se ele não estiver ativo:
`Xvfb :88 -screen 0 1024x900x24 -nolisten tcp`. Para a captura de tela:
`DISPLAY=:88 build/plugin_tests_artefacts/Release/plugin_tests build/interface.png`,
a partir da raiz do checkout.

Os processos e o display precisam ser iniciados novamente em uma nova máquina.
Use o checkout existente: cada tarefa da nuvem já é isolada, sem necessidade de worktree.
Para máquinas comuns, prefira os comandos de instalação normal do README.

## Compilação cruzada Windows

JUCE 8 não suporta MinGW. A tentativa com 8.0.4 confirmou essa restrição. O build
cruzado usa a versão compatível JUCE **7.0.12**, commit
`4f43011b96eb0636104cb3e433894cda98243626`, e a mesma aplicação C++.
JUCE 7 tem termos de licença distintos de JUCE 8: consulte a licença dessa versão
(GPLv3 ou licença JUCE aplicável). O build nativo recomendado no Windows continua
sendo Visual Studio 2022 com JUCE 8, conforme README.

No ambiente preparado, os wrappers em `/workspace/toolchain/bin` removem somente
os caminhos de headers e libs Linux para os compiladores MinGW. Isso evita misturar
headers nativos com Windows. As ferramentas auxiliares JUCE continuam nativas.

```sh
source /workspace/toolchain/activate.sh
export PATH=/workspace/toolchain/bin:$PATH
export CMAKE_BUILD_PARALLEL_LEVEL=2
cmake -S . -B build-win7 -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-x64.cmake \
  -DJUCE_SOURCE_DIR=/workspace/toolchain/JUCE7
cmake --build build-win7 --target VocalBalance_VST3 VocalBalance_Standalone --parallel 2
```

A geração automática do manifesto é desativada no build cruzado porque requer
executar um programa Windows. Hosts VST3 podem consultar a fábrica do plugin sem
esse arquivo. Um binário compilado não estabelece compatibilidade com todos os
DAWs; carregamento e automação em um DAW Windows precisam de validação real.

## Teste de interface nativa VST2

O teste `vst2_module` abre o editor numa janela X11 real. Além de Xvfb, é necessário
um gerenciador de janelas no display de teste. Nesta instância usamos:

```sh
mkdir -p /workspace/toolchain/wm-config /workspace/toolchain/wm-cache
DISPLAY=:88 XDG_CONFIG_HOME=/workspace/toolchain/wm-config \
  XDG_CACHE_HOME=/workspace/toolchain/wm-cache \
  dbus-run-session -- xfwm4 --compositor=off
```

Inicie esse processo separadamente e mantenha-o ativo ao executar CTest. Isso não
altera outro display. `xfwm4` já estava instalado. O teste de processamento não
necessita dispositivo de áudio físico; o host mínimo fornece os buffers.
