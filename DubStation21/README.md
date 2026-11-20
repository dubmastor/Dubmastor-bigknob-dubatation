# Dub Station 2.1 (FitaEco TE-12) — Tape Echo + Spring Reverb (VST3 / AU / Standalone)

Por DubMastor.

![Interface (prévia desenhada a partir do código)](docs/interface.png)

Plugin de delay de fita inspirado nos ecos de fita clássicos de 3 cabeças dos anos 70
(estilo RE-201). Código em C++17 com JUCE 8.

## Controles

| Knob | Função |
|---|---|
| INPUT | Ganho do pré-amplificador (satura a entrada) |
| MODE | Seletor de 12 posições (tabela abaixo) |
| REPEAT RATE | Velocidade da fita. Mudar durante a execução dobra o pitch, como fita de verdade |
| INTENSITY | Realimentação. Acima de ~0,8 (1 cabeça) ou ~0,7 (várias) entra em auto-oscilação |
| ECHO VOL / REVERB VOL | Volume dos ecos e da mola |
| GRAPHIC EQ | 6 faders (100, 250, 630, 1,6k, 4k, 10 kHz), ±12 dB, no eco + reverb. Duplo clique zera |
| WOW/FLUTTER | Instabilidade da fita (wow lento ~0,6 Hz + flutter ~7 Hz + deriva aleatória) |
| TAPE AGE | Fita gasta: mais chiado, mais saturação, cabeças mais escuras |
| DRY / OUTPUT | Nível do sinal seco e saída geral |

### Seletor de modo
| Modo | Cabeças | Reverb |
|---|---|---|
| 1 / 2 / 3 | 1 / 2 / 3 | não |
| 4 | 2+3 | não |
| 5 / 6 / 7 | 1 / 2 / 3 | sim |
| 8 | 1+2 | sim |
| 9 | 2+3 | sim |
| 10 | 1+3 | sim |
| 11 | 1+2+3 | sim |
| 12 | — (só reverb) | sim |

Tempos: cabeça 1 de 175 ms (lento) a 55 ms (rápido); cabeças 2 e 3 em 2× e 3×.

## Modelo de DSP (`Source/TapeEchoDSP.h`)
- Pré-amp com saturação `tanh` → cabeça de gravação com saturação assimétrica
- Loop de fita com interpolação Hermite; velocidade com inércia de motor (0,25 s)
- Perda de agudos nas cabeças dependente da velocidade e do desgaste da fita
- HPF no caminho de realimentação (evita acúmulo de grave, como no original)
- Reverb de mola: cascatas de all-pass dispersivas (o "drip") + duas molas acopladas com amortecimento e modulação
- Chiado de fita, wow & flutter modulando a velocidade de todas as cabeças juntas

## Download

O GitHub Actions compila sozinho a cada push nesta pasta (`.github/workflows/build-dubstation21.yml`):
- **Actions → "Build Dub Station 2.1" → última execução → Artifacts**: `DubStation21-Windows` e `DubStation21-macOS`.
- Para gerar uma release pública com os zips, crie uma tag `ds21-v2.1.0` (por exemplo) e envie: `git tag ds21-v2.1.0 && git push origin ds21-v2.1.0`.

Instalação: copie `FitaEco TE-12.vst3` para `C:\Program Files\Common Files\VST3` (Windows) ou
`~/Library/Audio/Plug-Ins/VST3` (Mac; o AU vai em `~/Library/Audio/Plug-Ins/Components`).

## Como compilar localmente

Requisitos: CMake ≥ 3.22, Git e um compilador C++17
(Windows: Visual Studio 2022 · macOS: Xcode · Linux: g++ + pacotes de dev do JUCE).

```bash
cd DubStation21
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

O CMake baixa o JUCE sozinho (ou use `-DJUCE_DIR=/caminho/JUCE`). O VST3 fica em
`build/FitaEco_artefacts/Release/VST3/`.

Linux — dependências:
```bash
sudo apt install libasound2-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev \
  libfreetype-dev libfontconfig1-dev libgl1-mesa-dev libxext-dev
```

## Teste offline do DSP
```bash
cd tests && g++ -std=c++17 -O2 render_test.cpp -o render_test && ./render_test
```
Roda todos os modos em 44,1/48/96 kHz, auto-oscilação e varredura de velocidade,
verifica estabilidade e grava WAVs.

---
Projeto independente; não é afiliado à Roland. "Space Echo" e "RE-201" são marcas da Roland Corporation,
citadas apenas como referência histórica.

## Fontes embutidas
- **Archivo Black** (placa metálica) — SIL Open Font License, ver `Resources/licenses/ArchivoBlack-OFL.txt`
- **TeX Gyre Chorus** (assinatura DubMastor) — GUST Font License, ver `Resources/licenses/TeXGyreChorus-GUST-FONT-LICENSE.txt`

As fontes são compiladas dentro do plugin (`juce_add_binary_data`), então a interface fica igual em qualquer sistema.
