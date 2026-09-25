# Computer Science for Digital Engineering

## Polar Skies & Moonlight Regalia

![C++](https://img.shields.io/badge/C%2B%2B-11-00599C?logo=cplusplus&logoColor=white)
![OpenGL](https://img.shields.io/badge/OpenGL-freeglut-5586A4?logo=opengl&logoColor=white)
![FreeCAD](https://img.shields.io/badge/FreeCAD-1.0-D40000?logo=freecad&logoColor=white)
![Platforms](https://img.shields.io/badge/platforms-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)

Practical assignment for the **Computer Science for Digital Engineering (with C++; CAD)** module,
BSc Computer Science and Digitisation.

Two projects built around one visual language, **the light of the polar sky**: a quiz game that
restores photographic plates of the aurora above an Arctic observatory, and a jewellery collection
that turns the same moonlight, stars and cold metal into wearable objects.

<p align="center">
  <img src="task1-polar-skies/screenshots/05b_level1_lastpiece.png" width="49%" alt="Polar Skies: the Level 1 archive plate fully restored">
  <img src="task2-moonlight-regalia/renders/scene_iso.png" width="49%" alt="Moonlight Regalia: the four pieces on their display stage">
</p>

| | Project | What it is | Built with |
|---|---|---|---|
| **Task 1** | [**Polar Skies**](task1-polar-skies/) | An interactive Arctic-observatory puzzle quiz game | C++11, OpenGL, freeglut |
| **Task 2** | [**Moonlight Regalia**](task2-moonlight-regalia/) | A celestial fantasy jewellery collection | FreeCAD 1.0 (Sketcher + Part Design) |

---

## Task 1 — Polar Skies

You are the night-shift observer at a remote Arctic research station. A storm has shattered the
station's telescope archive, and every polar-science question you answer correctly recovers one
piece of an archive plate. Restore **Plate I: The Great Bear** under the polar night to unlock the
dawn station and **Plate II: The Midnight Sun**, then finish the expedition.

- Animated aurora curtains, drifting snow, twinkling stars and a shaded observatory, all drawn from OpenGL primitives
- 12 questions over 2 levels, each followed by a "field note" fact
- 6-piece jigsaw reveal per plate using the OpenGL scissor test
- 20-second timer, score with streak bonus, heat-cell lives, colour feedback flashes, pause and F9 screenshots
- Level 2 swaps the whole scene for a rose-and-violet polar dawn with a brand-new plate

**Run it:** download [`task1-polar-skies/PolarSkies.exe`](task1-polar-skies/PolarSkies.exe) and
double-click it. It's a standalone 64-bit Windows build, so there's nothing to install.

➡️ Full details, controls and build instructions: [**task1-polar-skies/README.md**](task1-polar-skies/README.md)

---

## Task 2 — Moonlight Regalia

The ceremonial jewellery of an imagined lunar-elven court. Four pieces share one strict design
vocabulary of crescents, five-pointed stars, teardrops and orbs, finished in silver, moon-gold,
pearl and a single aurora-teal gemstone.

| Piece | Highlights |
|---|---|
| **Selene's Band** | Revolved gold band, crescent-pocketed bezel, eight polar-patterned star dimples |
| **Star-forged Circlet** | Eight star-crested spikes (polar pattern), moon-gold orbs, **SIDDHANT** engraved in the band |
| **Aurora Tear** | Revolved briolette gem held in a padded crescent cradle with a torus bail and silver beads |
| **Starlight Studs** | Star plate, pearl and post, with the second earring created as an App Link |

<p align="center">
  <img src="task2-moonlight-regalia/renders/obj_ring.png" width="32%" alt="Selene's Band">
  <img src="task2-moonlight-regalia/renders/obj_circlet_name.png" width="32%" alt="Star-forged Circlet with the engraved name">
  <img src="task2-moonlight-regalia/renders/pendant_studs.png" width="32%" alt="Aurora Tear pendant and Starlight Studs">
</p>

**Open it:** [`task2-moonlight-regalia/MoonlightRegalia.FCStd`](task2-moonlight-regalia/MoonlightRegalia.FCStd)
in FreeCAD 1.0 or newer. It opens straight onto the finished scene with colours applied.

➡️ Full details, feature tree and modelling workflow: [**task2-moonlight-regalia/README.md**](task2-moonlight-regalia/README.md)

---

## Learning outcomes

| Outcome | Where it is evidenced |
|---|---|
| **LO1** Write programs in C++ to solve complex problems | Task 1: finite state machine, question queue with re-queued misses, timer, scoring and a scissor-clipped puzzle reveal in one commented source file |
| **LO2** Use CAD technology for design and technical documentation | Task 2: sketch-first parametric bodies with a clean, named feature history, plus rendered views and FreeCAD window captures |
| **LO3** Use C++ and CAD for product innovation and digital manufacturing | A cross-platform game with a standalone Windows build, and watertight solids modelled in real millimetres that are ready for STL export and 3D-printed casting patterns |

## Report

[`report/CSDE_Practical_Assignment_Siddhant_Dane.docx`](report/CSDE_Practical_Assignment_Siddhant_Dane.docx)
covers the design logic, puzzle mechanics, modelling process and figures for both tasks
(2,100 words, 27 Harvard references).

## Repository layout

```
polar-skies-moonlight-regalia/
├── README.md                          ← you are here
├── .gitignore
├── report/
│   └── CSDE_Practical_Assignment_Siddhant_Dane.docx
├── task1-polar-skies/
│   ├── README.md                      ← game guide, controls, build steps
│   ├── main.cpp                       ← complete commented source (1,440 lines)
│   ├── PolarSkies.exe                 ← standalone Windows build
│   └── screenshots/                   ← 10 captures of every game state
└── task2-moonlight-regalia/
    ├── README.md                      ← collection guide, feature tree
    ├── MoonlightRegalia.FCStd         ← FreeCAD project (sketches, bodies, scene)
    └── renders/                       ← 13 renders and FreeCAD window captures
```

## Author

**Siddhant Dane** · BSc Computer Science and Digitisation

> **Academic integrity:** this repository contains assessed coursework. You're welcome to read it
> and learn from it, but please don't copy it into your own submission.

© 2026 Siddhant Dane. All rights reserved.
