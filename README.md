# doom-like-cpp

Prototipo inicial de FPS retro em C++ com raylib, CMake e raycasting basico.

## O que ja tem

- janela do jogo
- mapa 2D em matriz
- player com posicao, angulo e campo de visao
- movimentacao por teclado
- paredes em pseudo-3D com raycasting
- minimapa simples para depuracao

## Controles

- `W`, `A`, `S`, `D`: mover
- `Left`, `Right`: girar
- `Esc`: fechar

## Build

Se `raylib` ja estiver instalada no sistema, o CMake tenta usa-la primeiro.
Se nao estiver, o projeto faz download automatico da biblioteca via `FetchContent`.

```powershell
cmake -S . -B build
cmake --build build
```

Executavel gerado:

- `build/doom_like`
