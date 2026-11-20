# DubMastor Big Knob — King Tubby tribute 🟢🟡⚫

> 🎛️ **Novo neste repositório:** [**Dub Station 2.1**](DubStation21/) — eco de fita de 3 cabeças + reverb de mola, com EQ gráfico e visual de equipamento dos anos 70 (pasta `DubStation21/`).

**[⬇️ Baixar a versão mais nova](https://github.com/dubmastor/Dubmastor-bigknob-dubatation/releases/latest)** · instruções em [Download e instalação](#️-download-e-instalação-no-cubase)

Estação de dub num plugin só: **filtro big knob** inspirado no high-pass por degraus do King Tubby, **eco de fita** com cabeçotes estilo Space Echo, **sirene de dub** e **maximizer** no final.
Formatos: **VST3** (Windows/macOS, para o Cubase), **AU** (macOS) e versão **Standalone**.

Cadeia: entrada → filtro → seco + eco (com EQ de retorno) + sirene → **MAXIMIZER**.

## FILTER
| Controle | O que faz |
|---|---|
| **Big Knob** | Corte. Em STEPPED: OFF · 70 · 100 · 150 · 250 · 500 · 1k · 2k · 3k · 5k · 7.5k · 10k Hz |
| **STEPPED** | Degraus de mesa ou varredura contínua (20 Hz–20 kHz) |
| **LOW PASS** | Troca de high-pass para low-pass |
| **24 dB** | Corte suave (12 dB/oit) ou radical (24 dB/oit) |
| **Resonance / Drive / Filter Mix** | Pico no corte, saturação na entrada, mistura com o original |

## TAPE ECHO
| Controle | O que faz |
|---|---|
| **SYNC** | Ligado: **TIME** em divisões do andamento do Cubase (1/16, 1/8 T, 1/8, 1/8 D, 1/4, 1/4 D, 1/2). Desligado: **REPEAT RATE** livre (40–1500 ms), como no Space Echo — girar com eco rolando dá o efeito de pitch da fita |
| **HEADS** | Seletor de cabeçotes: 1, 2, 3, 1+2, 2+3, 1+3, 1+2+3. Cabeçote 2 = 2× o tempo, cabeçote 3 = 3× |
| **INTENSITY** | Realimentação. Acima de 100% entra em autooscilação controlada pela saturação |
| **TONE** | Escurece as repetições (filtro dentro do loop) |
| **WOW** | Oscilação de fita velha |
| **SEND** | Quanto do sinal entra no eco |
| **PING-PONG** | Repetições alternando esquerda/direita |
| **THROW** | Segure para jogar o som no eco (envio 100% só enquanto pressionado) |

### RETURN EQ (eco)
EQ nas repetições, com visor que desenha a curva: **HP** (corta grave, OFF a 2 kHz), **MID** (150 Hz – 6 kHz) com **GAIN** (±12 dB) para acentuar ou cavar um médio, e **LP** (corta agudo, 800 Hz a OFF). Duplo clique volta ao padrão.


## DUB SIREN
Sirene de sound system com delay próprio. Mesmo depois que você solta o FIRE, as repetições continuam.
| Controle | O que faz |
|---|---|
| **PITCH** | Nota base (80 Hz – 2.5 kHz) |
| **RATE** | Velocidade da modulação (0.1 – 25 Hz) |
| **DEPTH** | Quanto o pitch varia (até ±2 oitavas) |
| **LFO WAVE** | Chave seletora da forma da modulação: Sine, Triangle, Square (sirene de polícia), Saw Up, Saw Down (o "piu-piu" clássico). **Clique no símbolo** da onda para escolher, **clique na chave** para avançar, ou arraste |
| **DELAY / REPEATS** | Tempo (30–2000 ms) e realimentação do delay da sirene |
| **LEVEL** | Volume da sirene |
| **FIRE** | Segure para tocar. A modulação recomeça do início a cada disparo |
| **HOLD** | Mantém a sirene ligada sem segurar |

## OUTPUT LEVEL
Na coluna da direita, acima do MASTER.
Par de VU meters de ponteiro (L/R), com balística de VU e calibração 0 VU = -18 dBFS. A lâmpada vermelha acende acima de +3 VU.

## MAXIMIZER
Compressor de "cola" + limiter brickwall no fim da cadeia, controlado por um fader só, no estilo Ozone.
| Controle | O que faz |
|---|---|
| **Fader PUSH** (dourado) | Arraste **para baixo** para empurrar o volume (até +20 dB). A linha dourada mostra o threshold sobre o medidor IN: o que passar dela é segurado |
| **CEILING** | Teto da saída (-3 a 0 dBFS, padrão -0.3). O sinal nunca passa dele (linha vermelha tracejada no OUT) |
| **ON** | Liga/desliga (BYPASS). Duplo clique no fader volta para 0 |

Medidor moderno: barras de LED **IN** (L/R), **GR** (redução de ganho, em laranja) e **OUT** (L/R, com peak hold), mais leituras de **PUSH**, **GR**, **PEAK** e **LUFS-S** (loudness de curto prazo, janela de 3 s, ITU-R BS.1770).

O limiter usa 1,5 ms de lookahead. Essa latência é informada ao Cubase, que compensa sozinho.

A janela é **redimensionável**: arraste o canto inferior direito (50% a 150%).

## MASTER
**Dry** (zere para usar como send/FX Channel) e **Output**.

## ⬇️ Download e instalação no Cubase

**Página de download (sempre a versão mais nova):**
https://github.com/dubmastor/Dubmastor-bigknob-dubatation/releases/latest

Links diretos:
- Windows: https://github.com/dubmastor/Dubmastor-bigknob-dubatation/releases/latest/download/DubMastorBigKnob-Windows.zip
- Mac: https://github.com/dubmastor/Dubmastor-bigknob-dubatation/releases/latest/download/DubMastorBigKnob-macOS.zip

Não precisa de conta no GitHub para baixar.

### Windows
1. Baixe o **DubMastorBigKnob-Windows.zip** e descompacte (botão direito → *Extrair tudo*).
2. Copie a pasta **`DubMastor Big Knob.vst3`** para:
   `C:\Program Files\Common Files\VST3\`
   (o Windows pede permissão de administrador: clique em *Continuar*).
3. Abra o Cubase.

### Mac
1. Baixe o **DubMastorBigKnob-macOS.zip** (o Mac descompacta sozinho).
2. No Finder, aperte **Cmd+Shift+G**, cole `~/Library/Audio/Plug-Ins/VST3/` e copie para lá o **`DubMastor Big Knob.vst3`**.
3. Abra o **Terminal** e rode este comando para o macOS liberar o plugin:
   ```
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"DubMastor Big Knob.vst3"
   ```
4. Abra o Cubase.
   (Para Logic: copie o `DubMastor Big Knob.component` para `~/Library/Audio/Plug-Ins/Components/` e rode o mesmo comando trocando o caminho.)

### No Cubase
- O plugin aparece como **DubMastor Big Knob** (fabricante **DubMastor**), na categoria de efeitos.
- Se não aparecer: **Studio → VST Plug-in Manager** → refaça o scan. Se estiver na lista de bloqueados, clique em **Reactivate**.
- **Atualizando de uma versão anterior:** feche o Cubase, substitua o `.vst3` antigo pelo novo e abra de novo.

### Uso recomendado
Crie um **FX Channel** com o DubMastor Big Knob, **DRY em 0** e **SEND em 100%**, e mande bateria, skank e voz pelos sends. Ligue o **W** (Write) da pista para gravar os movimentos do big knob, THROW e FIRE.

## Como novas versões são geradas
Cada envio para a branch `main` compila o plugin automaticamente (aba **Actions**). Quando uma tag `v*` é criada (ex.: `v2.0.0`), os instaladores são publicados na página de **Releases**.

## Compilar no próprio computador (opcional)

Precisa de CMake 3.22+ e Visual Studio 2022 (Windows) ou Xcode (macOS):

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Estrutura

- `Source/TubbyFilter.h` — filtro SVF com saturação
- `Source/DubFX.h` — eco de fita multicabeçote, EQ de retorno, sirene, maximizer e medidor de loudness
- `Source/PluginProcessor.*` — parâmetros e processamento
- `Source/PluginEditor.*` — interface
- `tests/render_demo.cpp` — teste da resposta em frequência e geração de áudio demo

Feito com JUCE 8 (licença AGPLv3 para projetos de código aberto; para vender o plugin fechado é preciso licença comercial do JUCE).
