# Windows 11: erro 0xC0E90002 ao carregar VST2/VST3

## Diagnóstico confirmado

A mensagem genérica "Imagem Incorreta" traz o código específico **0xC0E90002**.
O SDK oficial da Microsoft define esse valor como
`STATUS_SYSTEM_INTEGRITY_POLICY_VIOLATION`: uma política de Application Control
bloqueou o arquivo. Fonte:
https://github.com/microsoft/win32metadata/blob/main/generation/WinSDK/RecompiledIdlHeaders/shared/ntstatus.h

No caso relatado, o usuário confirmou Windows 11 pessoal com Controle Inteligente
de Aplicativos ativado. Ambos os módulos distribuídos são PE32+ x64 e não têm
assinatura Authenticode. Esse conjunto é consistente com o bloqueio observado.

A validação anterior de arquitetura, ZIP e checksum não demonstrava aceitação
pela política de segurança do computador. Trocar o nome do arquivo ou gerar
novamente a mesma DLL sem assinatura não resolve essa exigência de confiança.

## O que foi testado depois do relato

Os arquivos Windows publicados foram carregados usando `LoadLibrary` e suas
funções exportadas em Wine 10. O host mínimo Windows do VST2 verificou áudio,
estado, Read/Write, sidechain e embedding do editor. O host mínimo Windows do
VST3 inicializou a fábrica e processou 500 blocos estéreo. Ambos passaram.

Wine não implementa a política Smart App Control do computador do usuário e não
substitui testes em Reaper/Studio One no Windows 11. Esses resultados verificam
carregamento e funcionamento básico dos arquivos, não liberação pela política.
Os testes agora também são compilados e executados pelo workflow Windows nativo.

## Distribuição com assinatura

Uma assinatura de código confiável é uma via de distribuição com identidade de
editor verificável. Ela não é a única explicação para um arquivo ser aceito:
o Controle Inteligente de Aplicativos também considera reputação. O DLL CHi Metric
Match Pro fornecido pelo usuário funciona no computador e também não contém
assinatura Authenticode. Portanto, não foi demonstrado que um certificado seja a
única solução para o caso. A aceitação final depende da política e do estado de
confiança do computador.

A comparação estática mostrou CHi Metric com linker MSVC 14.44 e JUCE 8.0.12,
contra os builds iniciais Vocal Balance com GNU/MinGW e JUCE 7.0.12. Isso é uma
diferença concreta de toolchain, não prova de causalidade do bloqueio. A versão
0.1.1 usa JUCE 8.0.12 e um build nativo Windows/Visual Studio 2022, com execução
dos hosts de teste Windows e publicação condicionada a todos os testes passarem.
Não são copiados assinatura, identidade, código ou recursos do DLL comparado.

O projeto agora inclui:

- `tools/Sign-WindowsArtifacts.ps1`: assina os dois módulos via Windows SDK
  SignTool, com SHA-256 e timestamp RFC3161, usando uma chave já disponível no
  repositório de certificados do Windows. Verifica EKU Code Signing, validade,
  RSA, cadeia/revogação, assinatura final e timestamp. Rejeita certificado
  autoassinado e falha se não puder validar o resultado.
- Workflow manual **Build and sign Windows plugins**, que compila e testa em
  Windows x64, assina, valida e só então empacota os dois formatos. Falta de
  certificado causa falha explícita; não há publicação automática de substituto
  sem assinatura como se estivesse corrigido.
- Builds normais identificados como `UNSIGNED-DEVELOPMENT`.

Não há certificado/chave de assinatura configurado neste ambiente. A assinatura
real não foi executada e nenhum pacote assinado foi publicado. Os downloads continuam sendo builds sem assinatura; a avaliação individual pelo
Controle Inteligente precisa ser verificada no computador do usuário.

### Usar um certificado existente no Windows

Com um certificado RSA de assinatura de código confiável e sua chave acessível
no armazenamento `Cert:\CurrentUser\My` (incluindo provedores de token compatíveis):

```powershell
./tools/Sign-WindowsArtifacts.ps1 -CertificateThumbprint 'THUMBPRINT_PUBLICO_DO_CERTIFICADO'
```

O thumbprint é um identificador público. A chave permanece no provedor de
certificados. O script não exporta a chave nem recebe senha na linha de comando.
Depois da assinatura é necessário criar novos ZIPs e novos checksums: os hashes
anteriores correspondem às imagens sem assinatura.

### GitHub Actions

Para um certificado cujo provedor permita uso por PFX, configure com segurança
em Settings → Secrets and variables → Actions do repositório:

- `WINDOWS_CODE_SIGN_PFX_BASE64`: PFX codificado em Base64.
- `WINDOWS_CODE_SIGN_PFX_PASSWORD`: senha desse PFX.

Execute o workflow manual **Build and sign Windows plugins**. A importação ocorre
no armazenamento temporário do runner; os arquivos/chaves importados são removidos
no bloco de limpeza. Não envie PFX, senhas, tokens ou chaves privadas em mensagens
ou commits. Certificados em HSM ou serviços de assinatura precisam da integração
apropriada do provedor e não devem ser exportados para contornar suas restrições.

## Verificação local sem alterar a proteção

No PowerShell, para o arquivo efetivamente instalado:

```powershell
Get-AuthenticodeSignature -LiteralPath 'C:\Program Files\Common Files\VST3\Vocal Balance.vst3\Contents\x86_64-win\Vocal Balance.vst3' |
  Format-List Status, StatusMessage, SignerCertificate
```

Para os binários atuais, é esperado `NotSigned`. O Visualizador de Eventos em
Applications and Services Logs → Microsoft → Windows → CodeIntegrity → Operational
pode mostrar o evento de bloqueio, frequentemente ID 3077. Confira se o caminho
corresponde ao Vocal Balance.

Depois de obter um pacote assinado, feche os DAWs e substitua somente as cópias
anteriores do plugin. Evite manter o ZIP extraído inteiro como uma segunda cópia
na pasta de busca: no VST3, copie a pasta `Vocal Balance.vst3` diretamente para
`C:\Program Files\Common Files\VST3\`. Uma cópia antiga ainda presente na busca
pode continuar produzindo o aviso, mesmo depois da instalação da nova.

## Reconstrução 0.2.0: DAW abre mas plugin ausente

O erro de política e a ausência na lista são sintomas diferentes. A reconstrução
usa novo identificador e nomes compactos para uma nova varredura, sem migrar
sessões da 0.1. O CI precisa encontrar e instanciar VST2 e VST3 no REAPER Windows
antes de publicar. Consulte `downloads/WINDOWS_NATIVE_BUILD.json` para os resultados.

Com o DAW fechado, mova as cópias antigas do Vocal Balance para fora das pastas
VST. Extraia o ZIP antes de instalar. Para VST3, a estrutura deve ser exatamente:

```
C:\Program Files\Common Files\VST3\VocalBalance.vst3\Contents\x86_64-win\VocalBalance.vst3
```

Não copie só o arquivo interno nem deixe o ZIP ou uma pasta de versão entre
`VST3` e `VocalBalance.vst3`. Para VST2, coloque `VocalBalance.dll` numa pasta
configurada em Preferences > Plug-ins > VST do REAPER (host x64). Instale o
Visual C++ Redistributable x64 oficial: https://aka.ms/vs/17/release/vc_redist.x64.exe .

REAPER: Preferences > Plug-ins > VST > Re-scan > Clear cache and re-scan.
Studio One: Options > Locations > VST Plug-ins, confirme o caminho VST2 e use
Reset Blocklist; reinicie para uma nova busca. Os nomes de menus podem variar.
Procure pelo nome **Vocal Balance** na lista de efeitos.

Se reaparecer 0xC0E90002, uma nova busca não resolve o bloqueio de Application
Control; consulte o diagnóstico de política acima. Esta versão não declara
aceitação pelo Smart App Control nem validação em Studio One 8.

## 0.2.1 — CRT estático, auditoria de PE e exports

O erro **0xC0E90002** é `STATUS_SYSTEM_INTEGRITY_POLICY_VIOLATION`.
Não é um diagnóstico de runtime ausente ou de manifest SxS. Um runtime ausente
costuma produzir `0xC0000135`; arquitetura incompatível, `0xC000007B`; falha de
activation context/SxS usa outros códigos, como `0xC0150002` ou Win32 14001.
Trocar /MD por /MT remove uma dependência real, mas não garante autorização
pela política de integridade/Smart App Control do computador.

A versão 0.2.0 usa /MD. A 0.2.1 fixa CMP0091 antes de `project()`, define
`CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>` antes de criar
qualquer target e recusa compilador Windows que não seja MSVC x64. Assim Release
usa /MT e Debug /MTd, incluindo JUCE, wrappers e testes. Os arquivos .vcxproj
correspondem a `RuntimeLibrary=MultiThreaded` na configuração Release.

O CI roda `dumpbin /headers`, `/dependents`, `/exports` e `mt.exe` em ambos os
formatos, antes (ZIP 0.2.0 preservado) e depois (0.2.1). Os relatórios em
`docs/windows-audit/` são gerados pelas ferramentas Microsoft reais. A publicação
é recusada se faltarem GetPluginFactory/InitDll/ExitDll no VST3, VSTPluginMain no
VST2, se a arquitetura não for 8664/x64, se houver import do CRT dinâmico ou
manifest referenciando um assembly VC CRT. Um manifest resource #2 ausente é
registrado como ausente, não inventado. As tabelas do binário e os testes de carga
verificam a saída final; não é preciso adicionar uma segunda fábrica VST3 ao JUCE.

O caminho relatado contém uma pasta adicional com sufixo `(1)`. Instale somente
a pasta `VocalBalance.vst3` diretamente em `C:\Program Files\Common Files\VST3\`.
Essa pasta adicional pode atrapalhar a busca, mas por si só não explica o código
de bloqueio por política. O arquivo PE interno permanece dentro do bundle;
não renomeie esse arquivo nem copie apenas ele para a raiz VST3.

Para verificar o computador real, cada ZIP inclui `Diagnose-WindowsInstall.ps1`
e os relatórios do build. No PowerShell, use `-Module` com o caminho exato do PE
instalado. O script lê hash, assinatura, arquitetura, Zone.Identifier, eventos
CodeIntegrity e disponibilidade das dependências do build em System32. Ele
não modifica configurações por padrão. `-Unblock` remove apenas a marca da Internet
do arquivo escolhido; isso não desativa nem ignora Application Control.
Nenhum certificado foi fabricado ou copiado. Os binários continuam sem assinatura.

Após baixar um ZIP conhecido e conferir seu SHA256, pode-se usar `Unblock-File
-LiteralPath 'C:\caminho\arquivo.zip'` antes de extrair. Feche o REAPER, substitua
as versões antigas e faça **Preferences > Plug-ins > VST > Re-scan > Clear cache
and re-scan**. Se persistir 0xC0E90002, os eventos CodeIntegrity do computador são
a evidência necessária para determinar a regra/assinatura/reputação envolvida.
A nuvem não possui acesso ao Windows do usuário para executar esse diagnóstico
ou confirmar que o erro desapareceu nele.
