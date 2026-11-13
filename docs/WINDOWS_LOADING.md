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

A próxima distribuição para esse cenário precisa de uma assinatura de código
confiável, validada e com timestamp. Um certificado autoassinado não equivale a
uma identidade de editor confiável para essa política. A aceitação final também
depende da política e do estado de confiança do computador.

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
real não foi executada e nenhum pacote assinado foi publicado. Os downloads
atuais continuam sendo builds de desenvolvimento sem assinatura.

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
