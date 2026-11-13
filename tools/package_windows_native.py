"""Package native Windows outputs only after the Windows integration tests pass."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import xml.etree.ElementTree as ET
import zipfile


def pe_info(path):
    data = path.read_bytes()
    offset = struct.unpack_from('<I', data, 0x3c)[0]
    if data[:2] != b'MZ' or data[offset:offset+4] != b'PE\0\0':
        raise ValueError(f'Invalid PE image: {path}')
    optional = offset + 24
    if struct.unpack_from('<H', data, offset+4)[0] != 0x8664 or struct.unpack_from('<H', data, optional)[0] != 0x20b:
        raise ValueError(f'Not a Windows x64 image: {path}')
    if data[optional+2] < 14:
        raise ValueError(f'Expected a Microsoft linker build: {path}')
    security_offset, security_size = struct.unpack_from('<II', data, optional+112+32)
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest(),
            'linker': f'{data[optional+2]}.{data[optional+3]}',
            'authenticode_table_present': bool(security_offset and security_size)}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--artifacts', type=Path, required=True)
    parser.add_argument('--test-report', type=Path, required=True)
    parser.add_argument('--reaper-report', type=Path, required=True)
    parser.add_argument('--juce', type=Path, required=True)
    parser.add_argument('--source-sha', required=True)
    parser.add_argument('--version', default='0.2.0')
    args = parser.parse_args()
    if os.name != 'nt':
        raise RuntimeError('Native release packaging must run on Windows')
    root = Path(__file__).resolve().parent.parent
    report = ET.parse(args.test_report).getroot()
    suites = [report] if report.tag == 'testsuite' else list(report.iter('testsuite'))
    tests = sum(int(s.get('tests', 0)) for s in suites)
    failures = sum(int(s.get('failures', 0)) + int(s.get('errors', 0)) + int(s.get('skipped', 0)) for s in suites)
    if tests < 4 or failures:
        raise RuntimeError(f'Required native tests did not all pass: tests={tests}, failures/skipped={failures}')
    reaper = json.loads(args.reaper_report.read_text(encoding='utf-8-sig'))
    if not reaper.get('passed') or reaper.get('vst2_instance', -1) < 0 or reaper.get('vst3_instance', -1) < 0:
        raise RuntimeError('REAPER did not discover and load both plugin formats')
    names = {'VST2': Path('VST2/VocalBalance.dll'),
             'VST3': Path('VST3/VocalBalance.vst3/Contents/x86_64-win/VocalBalance.vst3')}
    images = {fmt: pe_info(args.artifacts / path) for fmt, path in names.items()}
    metadata = {'version': args.version, 'source_commit': args.source_sha,
                'build_os': 'Windows x64', 'compiler': 'Microsoft Visual C++ / Visual Studio 2022',
                'juce': '8.0.12', 'native_tests_passed': tests,
                'reaper_windows': reaper, 'studio_one_and_smart_app_control': 'Requires user validation', 'images': images}
    output = root / 'downloads'
    output.mkdir(exist_ok=True)
    for fmt in names:
        filename = f'VocalBalance-{args.version}-{fmt}-Windows-x64-MSVC.zip'
        with tempfile.TemporaryDirectory() as tmp:
            stage = Path(tmp)
            if fmt == 'VST2':
                shutil.copy2(args.artifacts / names[fmt], stage / 'VocalBalance.dll')
            else:
                shutil.copytree(args.artifacts / 'VST3/VocalBalance.vst3', stage / 'VocalBalance.vst3')
            licenses = stage / 'licenses'
            licenses.mkdir()
            shutil.copy2(args.juce / 'LICENSE.md', licenses / 'JUCE-LICENSE.md')
            shutil.copy2(root / 'third_party/xaymar-vst2/LICENSE', licenses / 'xaymar-vst2-LICENSE.txt')
            sdk = args.juce / 'modules/juce_audio_processors_headless/format_types/VST3_SDK'
            if not sdk.is_dir():
                sdk = args.juce / 'modules/juce_audio_processors/format_types/VST3_SDK'
            for name in ['LICENSE.txt', 'VST3_License_Agreement.pdf']:
                if (sdk / name).is_file():
                    shutil.copy2(sdk / name, licenses / ('VST3-' + name))
            for name in ['LICENSE', 'docs/THIRD_PARTY.md', 'docs/WINDOWS_LOADING.md']:
                shutil.copy2(root / name, stage / Path(name).name)
            (stage / 'BUILD-INFO.json').write_text(json.dumps(metadata, indent=2), encoding='utf-8')
            instruction = (r'Copie VocalBalance.dll para a pasta VST2 configurada no DAW.' if fmt == 'VST2'
                           else r'Copie a pasta inteira VocalBalance.vst3 para C:\Program Files\Common Files\VST3\ .')
            (stage / 'INSTALAR.txt').write_text(
                f'VOCAL BALANCE {args.version} - {fmt} WINDOWS X64 - MSVC\n\n'
                'Feche os DAWs e remova as copias anteriores do Vocal Balance das pastas de busca.\n'
                + instruction + '\nAtualize a busca de plugins.\n\n'
                'Compilado e testado nativamente no Windows com Visual C++ e JUCE 8.0.12.\n'
                'Requer Microsoft Visual C++ Redistributable x64 (familia 2015-2022).\n'
                'Build sem assinatura; a aceitacao pelo Smart App Control ainda depende da politica e reputacao.\n'
                'Veja BUILD-INFO.json e WINDOWS_LOADING.md.\n', encoding='utf-8')
            with zipfile.ZipFile(output / filename, 'w', zipfile.ZIP_DEFLATED) as archive:
                for path in sorted(stage.rglob('*')):
                    if path.is_file():
                        archive.write(path, path.relative_to(stage))
        with zipfile.ZipFile(output / filename) as archive:
            if archive.testzip() is not None:
                raise RuntimeError('Corrupt ZIP: ' + filename)
    (output / 'WINDOWS_NATIVE_BUILD.json').write_text(json.dumps(metadata, indent=2), encoding='utf-8')
    source_zip = output / f'VocalBalance-{args.version}-Sources.zip'
    files = subprocess.check_output(['git', 'ls-files'], cwd=root, text=True).splitlines()
    with zipfile.ZipFile(source_zip, 'w', zipfile.ZIP_DEFLATED) as archive:
        for name in files:
            if not name.startswith(('downloads/', 'build-reports/')) and (root / name).is_file():
                archive.write(root / name, name)
    (output / 'SHA256SUMS.txt').write_text(''.join(hashlib.sha256(path.read_bytes()).hexdigest() + '  ' + path.name + '\n'
        for path in sorted(output.glob('*.zip'))), encoding='utf-8')
    base = 'https://github.com/aulamix001/Vocal-Rider/raw/refs/heads/main/downloads/'
    links = '\n\n'.join(f'[**Baixar {fmt} Windows x64 — MSVC {args.version}**]({base}VocalBalance-{args.version}-{fmt}-Windows-x64-MSVC.zip)' for fmt in names)
    section = ('## Download compilado — Windows x64\n\n' + links + '\n\n'
               f'Versão **{args.version}**, compilada no Windows com **Visual C++ / JUCE 8.0.12**, '
               f'com **{tests}/{tests} testes nativos aprovados** e VST2/VST3 encontrados e carregados no **REAPER {reaper["version"]}**.\n\n'
               'VST2: copie a DLL para a pasta VST2 do DAW. VST3: copie a pasta `.vst3` inteira para '
               '`C:\\Program Files\\Common Files\\VST3\\`. Remova as cópias antigas antes de atualizar a busca.\n\n'
               '**Sem assinatura Authenticode:** o carregamento com Controle Inteligente de Aplicativos '
               'ativado ainda precisa ser confirmado no seu computador. [Comparação e diagnóstico](docs/WINDOWS_LOADING.md).\n\n'
               '[Informações do build](downloads/WINDOWS_NATIVE_BUILD.json) · '
               '[Checksums](downloads/SHA256SUMS.txt)\n\n')
    readme = root / 'README.md'
    text = readme.read_text(encoding='utf-8')
    start = text.index('## Download compilado — Windows x64')
    end = text.index('## Compilar no Windows', start)
    readme.write_text(text[:start] + section + text[end:], encoding='utf-8')
    (output / 'README.md').write_text('# Vocal Balance — Windows x64 nativo\n\n' + links + '\n\n'
        f'Versão {args.version}. Visual C++ e JUCE 8.0.12; {tests}/{tests} testes em Windows aprovados.\n'
        f'REAPER {reaper["version"]}: VST2 e VST3 encontrados e instanciados. Studio One e Smart App Control ainda precisam de validação no computador do usuário.\n\n'
        '[Dados do build](WINDOWS_NATIVE_BUILD.json) · [SHA-256](SHA256SUMS.txt)\n', encoding='utf-8')
    print(json.dumps(metadata, indent=2))


if __name__ == '__main__':
    main()
