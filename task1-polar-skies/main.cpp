/* ============================================================================
   POLAR SKIES - An Arctic Observatory Puzzle Quiz
   ----------------------------------------------------------------------------
   Module  : Computer Science for Digital Engineering (C++ / OpenGL)
   Author  : Siddhant Dane
   Build   : single-file C++ (C++11) + OpenGL immediate mode + (Free)GLUT

   CONCEPT
   You are the night-shift observer at a remote Arctic research station.
   The station's "telescope archive plates" (the puzzles) were shattered by
   a storm. Answering polar-science questions correctly recovers the archive
   one piece at a time. Restore Plate I (The Great Bear) to unlock the dawn
   station and Plate II (The Midnight Sun), then finish the expedition.

   HOW THE BRIEF IS COVERED
   1. Game environment ..... animated aurora, snowfall, twinkling stars,
                             observatory dome, pines, snowman, signpost.
   2. Puzzle design ........ each archive plate is a 3x2 puzzle; one tile is
                             revealed per correct answer (glScissor reveal).
   3. Q&A logic ............ 12 questions (6 per level), full correct /
                             incorrect / timeout handling with explanations.
   4. Interactive gameplay . keyboard only: 1/2/3 answer, ENTER continue,
                             P pause, R restart, F9 screenshot, ESC quit.
   5. Scene progression .... Level 2 uses a NEW background (polar dawn) and a
                             NEW puzzle; a completion screen ends the game.
   6. 3D appearance ........ layered shading on the dome, moon and snowman,
                             soft drop shadows, parallax mountain layers.
   7. Name engraving ....... carved wooden signpost + HUD observer credit.
   Bonus features .......... 20 s question timer, score with streak bonus,
                             heat (lives) system, colour feedback flashes,
                             continuous animation, pause, screenshot key.

   BUILD (see README.txt for the full notes)
     Windows (MSYS2/MinGW): g++ main.cpp -o PolarSkies.exe -lfreeglut -lopengl32 -lwinmm -lgdi32
     Linux                : g++ main.cpp -o polarskies -lglut -lGL -lm
     macOS                : clang++ main.cpp -o polarskies -framework OpenGL -framework GLUT
   ============================================================================ */

#ifdef __APPLE__
  #define GL_SILENCE_DEPRECATION
  #include <GLUT/glut.h>
#else
  #include <GL/glut.h>
#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

/* ----------------------------------------------------------------------------
   1. SMALL UTILITIES AND GLOBAL STATE
   ----------------------------------------------------------------------------
   The game draws in a fixed logical space of 1280 x 720 "design pixels".
   The reshape handler stretches this space onto the real window, so every
   coordinate below can stay constant no matter how the window is resized.   */

static const float LW = 1280.0f;              /* logical width               */
static const float LH = 720.0f;               /* logical height              */
static int   winW = 1280, winH = 720;         /* real window size (pixels)   */
static const float PI = 3.14159265358979f;

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static float lerpf (float a, float b, float t)   { return a + (b - a) * t; }
static float frand (float a, float b)            { return a + (b - a) * (rand() / (float)RAND_MAX); }

/* Game states form a tiny finite state machine.                              */
enum GameState { S_MENU, S_PLAYING, S_FEEDBACK, S_LEVEL_DONE, S_GAME_DONE, S_GAME_OVER };
static GameState state = S_MENU;

static int   level          = 0;      /* 0 = Polar Night, 1 = Polar Dawn     */
static int   score          = 0;
static int   levelStartScore= 0;      /* restored if the level is retried    */
static int   streak         = 0;      /* consecutive correct answers         */
static int   bestStreak     = 0;
static int   wrongCount     = 0;
static int   lives          = 3;      /* "heat cells" - lose one per mistake */
static int   revealed       = 0;      /* puzzle tiles recovered this level   */
static const int TILES      = 6;      /* 3 columns x 2 rows per plate        */
static bool  paused         = false;

static float now            = 0.0f;   /* seconds since program start         */
static float lastFrame      = 0.0f;
static float timeLeft       = 20.0f;  /* per-question countdown              */
static const float Q_TIME   = 20.0f;

/* Feedback (shown after every answer)                                        */
static bool        fbCorrect   = false;
static bool        fbTimeout   = false;
static int         fbChosen    = -1;
static int         fbQIdx      = 0;       /* which question was just answered */
static int         fbGained    = 0;
static float       fbStart     = 0.0f;
static float       flashStart  = -10.0f;  /* screen-edge colour flash        */
static bool        flashGood   = false;
static float       tileReveal[TILES];     /* reveal time per tile (for fade) */

/* ----------------------------------------------------------------------------
   2. QUESTION BANK
   ----------------------------------------------------------------------------
   Two themed rounds of six questions. Wrong answers are pushed back into the
   queue, so a plate can always be completed - mistakes cost heat and score,
   not progress. Every question carries a short fact for the feedback panel. */

struct Question {
    const char* text;
    const char* opt[3];
    int         correct;      /* index 0..2                                  */
    const char* fact;
};

static const Question BANK[2][6] = {
{ /* Level 1 - THE POLAR NIGHT (aurora and the northern sky) */
  { "What causes the aurora borealis?",
    { "Charged solar particles striking the atmosphere",
      "Sunlight reflecting off the polar ice cap",
      "Lightning trapped inside polar clouds" }, 0,
    "Solar wind particles collide with oxygen and nitrogen roughly 100 km up, making the sky glow." },
  { "Which gas paints the aurora its famous green colour?",
    { "Hydrogen", "Oxygen", "Carbon dioxide" }, 1,
    "Excited atomic oxygen emits the classic green light at a wavelength of 557.7 nanometres." },
  { "The Big Dipper is part of which constellation?",
    { "Orion", "Cassiopeia", "Ursa Major" }, 2,
    "Ursa Major is Latin for Great Bear - the Dipper forms the bear's tail and hindquarters." },
  { "Polaris, the North Star, sits almost exactly above the...",
    { "North Pole", "Arctic Circle", "Equator" }, 0,
    "Polaris stays within about one degree of the celestial pole, so it barely moves all night." },
  { "Which region of space shields Earth from most of the solar wind?",
    { "The ozone layer", "The magnetosphere", "The stratosphere" }, 1,
    "Earth's magnetic field funnels solar particles toward the poles - that is why auroras love high latitudes." },
  { "Auroras seen in the Southern Hemisphere are called...",
    { "Aurora australis", "Aurora antarctica", "Aurora polaris" }, 0,
    "The aurora australis mirrors the northern lights and is best watched from Antarctica and Tasmania." }
},
{ /* Level 2 - THE POLAR DAWN (Arctic geography and wildlife) */
  { "Roughly how much of an iceberg hides below the waterline?",
    { "About 10 percent", "About half", "About 90 percent" }, 2,
    "Ice is only a little less dense than seawater, so nearly nine tenths of a berg floats unseen." },
  { "Which ocean surrounds the North Pole?",
    { "The Arctic Ocean", "The North Atlantic", "The Southern Ocean" }, 0,
    "The Arctic Ocean is the smallest and shallowest of the five oceans - and much of it is frozen." },
  { "Beneath the white fur, a polar bear's skin is actually...",
    { "Pink", "Black", "Pale blue" }, 1,
    "Black skin absorbs sunlight, while hollow transparent hairs scatter light so the bear looks white." },
  { "What is permafrost?",
    { "Ground frozen for at least two years", "Freshly compacted snow", "A slow-moving glacier" }, 0,
    "Some Siberian permafrost has stayed frozen for hundreds of thousands of years." },
  { "At the North Pole in midsummer, the Sun...",
    { "Sets twice a day", "Never rises", "Never sets" }, 2,
    "The midnight sun circles the horizon for months - the pole gets one long day and one long night a year." },
  { "Which Arctic animal makes the longest migration on Earth?",
    { "The Arctic tern", "The caribou", "The snowy owl" }, 0,
    "Arctic terns commute between the Arctic and Antarctica - up to 70,000 km in a single year." }
} };

static std::vector<int> queueQ;   /* indices into BANK[level], front = next  */

/* ----------------------------------------------------------------------------
   3. LEVEL PALETTES
   ----------------------------------------------------------------------------
   Level 2 re-uses the whole scene pipeline with a different palette plus a
   sun instead of a moon - an economical way to deliver a brand new mood.   */

struct Palette {
    float skyTop[3], skyBot[3];
    float mountBack[3], mountFront[3];
    float snowTop[3], snowBot[3];
    float aurA[3], aurB[3];          /* aurora base / tip colours            */
    float aurAlpha;
    float starAlpha;
    bool  sun;                        /* false: moon, true: dawn sun         */
    const char* name;
};

static const Palette PAL[2] = {
  { {0.02f,0.04f,0.14f}, {0.07f,0.13f,0.30f},
    {0.11f,0.15f,0.30f}, {0.17f,0.23f,0.40f},
    {0.72f,0.79f,0.92f}, {0.52f,0.60f,0.80f},
    {0.10f,0.95f,0.55f}, {0.45f,0.35f,0.95f}, 0.42f, 1.0f, false,
    "POLAR NIGHT" },
  { {0.16f,0.10f,0.30f}, {0.98f,0.55f,0.42f},
    {0.36f,0.20f,0.38f}, {0.55f,0.32f,0.46f},
    {0.99f,0.88f,0.86f}, {0.88f,0.66f,0.70f},
    {0.98f,0.45f,0.60f}, {0.55f,0.30f,0.90f}, 0.26f, 0.35f, true,
    "POLAR DAWN" }
};

/* Ambient particles ---------------------------------------------------------*/
struct Flake { float x, y, r, v, sway, phase; };
struct Star  { float x, y, r, speed, phase;   };
static std::vector<Flake> snowflakes;
static std::vector<Star>  stars;

/* Puzzle frame geometry (logical coordinates) -------------------------------*/
static const float PZ_X = 764, PZ_Y = 214, PZ_W = 472, PZ_H = 372; /* image  */
static const float PZ_BORDER = 14;                                  /* frame */

/* ----------------------------------------------------------------------------
   4. LOW-LEVEL DRAWING HELPERS
   --------------------------------------------------------------------------*/

static void drawCircle(float cx, float cy, float r, int seg = 40, bool filled = true)
{
    glBegin(filled ? GL_TRIANGLE_FAN : GL_LINE_LOOP);
    if (filled) glVertex2f(cx, cy);
    for (int i = 0; i <= seg; ++i) {
        float a = i * 2.0f * PI / seg;
        glVertex2f(cx + cosf(a) * r, cy + sinf(a) * r);
    }
    glEnd();
}

static void drawEllipse(float cx, float cy, float rx, float ry, int seg = 40)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= seg; ++i) {
        float a = i * 2.0f * PI / seg;
        glVertex2f(cx + cosf(a) * rx, cy + sinf(a) * ry);
    }
    glEnd();
}

/* Rounded rectangle built from a fan per corner - used for every UI panel.  */
static void drawRoundRect(float x, float y, float w, float h, float r, bool filled = true)
{
    const int seg = 8;
    float cx[4] = { x + w - r, x + r, x + r, x + w - r };
    float cy[4] = { y + h - r, y + h - r, y + r, y + r };
    float start[4] = { 0, 0.5f * PI, PI, 1.5f * PI };
    glBegin(filled ? GL_TRIANGLE_FAN : GL_LINE_LOOP);
    if (filled) glVertex2f(x + w * 0.5f, y + h * 0.5f);
    for (int c = 0; c < 4; ++c)
        for (int i = 0; i <= seg; ++i) {
            float a = start[c] + i * (0.5f * PI) / seg;
            glVertex2f(cx[c] + cosf(a) * r, cy[c] + sinf(a) * r);
        }
    if (filled) glVertex2f(cx[0] + r, cy[0]);
    glEnd();
}

/* Five (or n) pointed star used for gems, sparkles and the dawn sun rays.   */
static void drawStarShape(float cx, float cy, float rOut, float rIn, int points, float rot)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= points * 2; ++i) {
        float r = (i % 2 == 0) ? rOut : rIn;
        float a = rot + i * PI / points;
        glVertex2f(cx + cosf(a) * r, cy + sinf(a) * r);
    }
    glEnd();
}

/* Bitmap text helpers (fixed UI font).                                      */
static float textWidth(void* font, const char* s)
{
    float w = 0;
    for (const char* c = s; *c; ++c) w += glutBitmapWidth(font, *c);
    return w;
}
static void text(float x, float y, void* font, const char* s)
{
    glRasterPos2f(x, y);
    for (const char* c = s; *c; ++c) glutBitmapCharacter(font, *c);
}
static void textCentered(float cx, float y, void* font, const char* s)
{
    text(cx - textWidth(font, s) * 0.5f, y, font, s);
}

/* Stroke text (scalable vector font) for titles and the engraved signpost.  */
static float strokeWidth(const char* s)
{
    float w = 0;
    for (const char* c = s; *c; ++c) w += glutStrokeWidth(GLUT_STROKE_ROMAN, *c);
    return w;
}
static void strokeText(float x, float y, float scale, float thickness, const char* s)
{
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1);
    glLineWidth(thickness);
    for (const char* c = s; *c; ++c) glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
    glPopMatrix();
    glLineWidth(1);
}
static void strokeCentered(float cx, float y, float scale, float thickness, const char* s)
{
    strokeText(cx - strokeWidth(s) * scale * 0.5f, y, scale, thickness, s);
}

/* Soft drop shadow: three stacked translucent ellipses give a blurred edge.
   These fake shadows are one of the layered "3D appearance" tricks.         */
static void dropShadow(float cx, float cy, float rx, float ry)
{
    glColor4f(0.05f, 0.08f, 0.18f, 0.18f); drawEllipse(cx, cy, rx * 1.25f, ry * 1.35f);
    glColor4f(0.05f, 0.08f, 0.18f, 0.22f); drawEllipse(cx, cy, rx, ry);
    glColor4f(0.05f, 0.08f, 0.18f, 0.28f); drawEllipse(cx, cy, rx * 0.65f, ry * 0.7f);
}

/* Six-armed snowflake icon for the heat/lives display.                      */
static void snowflakeIcon(float cx, float cy, float r, bool alive)
{
    if (alive) glColor4f(0.65f, 0.92f, 1.0f, 0.95f);
    else       glColor4f(0.5f, 0.55f, 0.7f, 0.30f);
    glLineWidth(2);
    glBegin(GL_LINES);
    for (int i = 0; i < 6; ++i) {
        float a = i * PI / 3;
        float dx = cosf(a), dy = sinf(a);
        glVertex2f(cx, cy); glVertex2f(cx + dx * r, cy + dy * r);
        /* small side branches */
        glVertex2f(cx + dx * r * 0.55f, cy + dy * r * 0.55f);
        glVertex2f(cx + dx * r * 0.55f + cosf(a + 0.7f) * r * 0.3f,
                   cy + dy * r * 0.55f + sinf(a + 0.7f) * r * 0.3f);
        glVertex2f(cx + dx * r * 0.55f, cy + dy * r * 0.55f);
        glVertex2f(cx + dx * r * 0.55f + cosf(a - 0.7f) * r * 0.3f,
                   cy + dy * r * 0.55f + sinf(a - 0.7f) * r * 0.3f);
    }
    glEnd();
    glLineWidth(1);
}

/* ----------------------------------------------------------------------------
   5. ENVIRONMENT - the living Arctic backdrop
   --------------------------------------------------------------------------*/

static void drawSky(const Palette& p)
{
    glBegin(GL_QUADS);                       /* vertical gradient            */
    glColor3fv(p.skyBot); glVertex2f(0, 0);   glVertex2f(LW, 0);
    glColor3fv(p.skyTop); glVertex2f(LW, LH); glVertex2f(0, LH);
    glEnd();
}

static void drawStarfield(const Palette& p)
{
    if (p.starAlpha <= 0.01f) return;
    for (size_t i = 0; i < stars.size(); ++i) {
        const Star& s = stars[i];
        float tw = 0.55f + 0.45f * sinf(now * s.speed + s.phase);
        glColor4f(0.9f, 0.95f, 1.0f, p.starAlpha * tw * 0.9f);
        drawCircle(s.x, s.y, s.r * (0.8f + 0.3f * tw), 10);
        if (s.r > 1.9f) {                    /* sparkle cross on big stars   */
            glColor4f(0.9f, 0.97f, 1.0f, p.starAlpha * tw * 0.5f);
            glLineWidth(1);
            glBegin(GL_LINES);
            glVertex2f(s.x - s.r * 3, s.y); glVertex2f(s.x + s.r * 3, s.y);
            glVertex2f(s.x, s.y - s.r * 3); glVertex2f(s.x, s.y + s.r * 3);
            glEnd();
        }
    }
    /* three slowly spinning "hero" stars give the sky a focal sparkle       */
    const float hero[3][2] = { {150, 655}, {700, 620}, {1150, 668} };
    for (int i = 0; i < 3; ++i) {
        float tw = 0.6f + 0.4f * sinf(now * 1.6f + i * 2.4f);
        glColor4f(0.95f, 0.98f, 1.0f, p.starAlpha * tw * 0.9f);
        drawStarShape(hero[i][0], hero[i][1], 7.5f + tw * 2, 2.6f, 4, now * 0.25f + i);
    }
}

/* The aurora: translucent quad-strip curtains riding slow sine waves.
   Bright at the lower hem, fading upward - drawn twice per band with a thin
   glowing base line, which reads convincingly as curtain light.             */
static void drawAurora(const Palette& p)
{
    for (int band = 0; band < 3; ++band) {
        float base  = 470 + band * 62;
        float amp   = 26 + band * 9;
        float speed = 0.35f + band * 0.11f;
        float height= 120 - band * 18;
        float alpha = p.aurAlpha * (1.0f - band * 0.22f);

        glBegin(GL_QUAD_STRIP);
        for (float x = -40; x <= LW + 40; x += 32) {
            float wave = sinf(x * 0.0075f + now * speed + band * 2.1f) * amp
                       + sinf(x * 0.021f - now * speed * 0.6f) * amp * 0.35f;
            float y0 = base + wave;
            glColor4f(p.aurA[0], p.aurA[1], p.aurA[2], alpha);
            glVertex2f(x, y0);
            glColor4f(p.aurB[0], p.aurB[1], p.aurB[2], 0.0f);
            glVertex2f(x, y0 + height);
        }
        glEnd();

        glLineWidth(2.5f);                   /* glowing hem                  */
        glBegin(GL_LINE_STRIP);
        glColor4f(p.aurA[0], p.aurA[1], p.aurA[2], alpha * 0.9f);
        for (float x = -40; x <= LW + 40; x += 32) {
            float wave = sinf(x * 0.0075f + now * speed + band * 2.1f) * amp
                       + sinf(x * 0.021f - now * speed * 0.6f) * amp * 0.35f;
            glVertex2f(x, base + wave);
        }
        glEnd();
        glLineWidth(1);
    }
}

/* Moon with halo, craters and a shaded terminator (layered-circle 3D).      */
static void drawMoon()
{
    float cx = 415, cy = 585, r = 44;
    for (int i = 4; i >= 1; --i) {                       /* halo             */
        glColor4f(0.85f, 0.9f, 1.0f, 0.05f);
        drawCircle(cx, cy, r + i * 14);
    }
    glColor3f(0.93f, 0.95f, 0.99f); drawCircle(cx, cy, r, 48);
    glColor4f(0.72f, 0.77f, 0.88f, 1.0f);                /* craters          */
    drawCircle(cx - 14, cy + 10, 7); drawCircle(cx + 12, cy - 6, 9);
    drawCircle(cx + 2,  cy + 22, 4); drawCircle(cx - 20, cy - 16, 5);
    glColor4f(0.10f, 0.14f, 0.30f, 0.22f);               /* terminator shade */
    drawCircle(cx - 12, cy + 6, r * 1.02f);
    glColor3f(0.95f, 0.97f, 1.0f);                       /* re-light rim     */
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx + r * 0.35f, cy - r * 0.2f);
    for (int i = 0; i <= 40; ++i) {
        float a = -0.9f + i * 1.8f / 40;
        glVertex2f(cx + cosf(a) * r * 0.98f, cy + sinf(a) * r * 0.98f);
    }
    glEnd();
}

/* Low dawn sun with warm halo and long horizontal light streaks.            */
static void drawDawnSun()
{
    float cx = 300, cy = 330, r = 40;
    for (int i = 5; i >= 1; --i) {
        glColor4f(1.0f, 0.75f, 0.45f, 0.06f);
        drawCircle(cx, cy, r + i * 22);
    }
    glColor4f(1.0f, 0.62f, 0.35f, 0.35f);                /* light streaks    */
    glBegin(GL_QUADS);
    for (int i = 0; i < 3; ++i) {
        float y = cy - 8 + i * 16, hw = 200 - i * 40;
        glVertex2f(cx - hw, y - 2); glVertex2f(cx + hw, y - 2);
        glVertex2f(cx + hw, y + 2); glVertex2f(cx - hw, y + 2);
    }
    glEnd();
    glColor3f(1.0f, 0.86f, 0.55f); drawCircle(cx, cy, r, 48);
    glColor4f(1.0f, 0.97f, 0.85f, 0.9f); drawCircle(cx - 8, cy + 8, r * 0.55f, 32);
}

static void drawMountains(const Palette& p)
{
    glColor3fv(p.mountBack);                              /* far ridge       */
    glBegin(GL_TRIANGLE_STRIP);
    glVertex2f(0, 170);    glVertex2f(0, 330);  glVertex2f(190, 170);
    glVertex2f(240, 400);  glVertex2f(430, 170);glVertex2f(560, 445);
    glVertex2f(760, 170);  glVertex2f(880, 380);glVertex2f(1060, 170);
    glVertex2f(1150, 420); glVertex2f(1280, 170);glVertex2f(1280, 340);
    glEnd();
    glColor3fv(p.mountFront);                             /* near ridge      */
    glBegin(GL_TRIANGLE_STRIP);
    glVertex2f(0, 170);   glVertex2f(0, 280);  glVertex2f(150, 170);
    glVertex2f(330, 355); glVertex2f(520, 170);glVertex2f(700, 330);
    glVertex2f(900, 170); glVertex2f(1010, 350);glVertex2f(1180, 170);
    glVertex2f(1280, 300);glVertex2f(1280, 170);glVertex2f(640, 170);
    glEnd();
    /* snow caps: small bright triangles pinned on the near peaks            */
    glColor4f(0.95f, 0.97f, 1.0f, 0.85f);
    glBegin(GL_TRIANGLES);
    glVertex2f(305, 329); glVertex2f(330, 355); glVertex2f(357, 327);
    glVertex2f(676, 306); glVertex2f(700, 330); glVertex2f(726, 304);
    glVertex2f(986, 325); glVertex2f(1010, 350);glVertex2f(1036, 323);
    glEnd();
}

static void drawGround(const Palette& p)
{
    glBegin(GL_QUADS);                        /* main snowfield gradient     */
    glColor3fv(p.snowBot); glVertex2f(0, 150);  glVertex2f(LW, 150);
    glColor3fv(p.snowTop); glVertex2f(LW, 258); glVertex2f(0, 242);
    glEnd();
    glColor4f(p.snowTop[0], p.snowTop[1], p.snowTop[2], 0.65f);
    glBegin(GL_TRIANGLE_STRIP);               /* soft foreground drift       */
    for (float x = 0; x <= LW; x += 64) {
        glVertex2f(x, 150);
        glVertex2f(x, 176 + sinf(x * 0.013f) * 9);
    }
    glEnd();
}

/* Observatory: shaded "cylinder" base + shaded dome + telescope + windows.  */
static void drawObservatory()
{
    float bx = 205, by = 245, bw = 190, bh = 115;   /* base rectangle        */
    float domeR = 92;
    float cxm = bx + bw / 2;

    dropShadow(cxm + 14, by - 4, 150, 16);

    /* base drawn as 8 vertical slices whose shade follows a cosine curve -
       a classic layered-shape trick that makes a flat wall read as round.   */
    int slices = 8;
    for (int i = 0; i < slices; ++i) {
        float f0 = (float)i / slices, f1 = (float)(i + 1) / slices;
        float shade = 0.55f + 0.45f * sinf(f0 * PI);
        glColor3f(0.30f * shade + 0.08f, 0.36f * shade + 0.10f, 0.52f * shade + 0.14f);
        glBegin(GL_QUADS);
        glVertex2f(bx + bw * f0, by); glVertex2f(bx + bw * f1, by);
        glVertex2f(bx + bw * f1, by + bh); glVertex2f(bx + bw * f0, by + bh);
        glEnd();
    }
    glColor3f(0.16f, 0.20f, 0.34f);                /* roof rim               */
    glBegin(GL_QUADS);
    glVertex2f(bx - 8, by + bh); glVertex2f(bx + bw + 8, by + bh);
    glVertex2f(bx + bw + 8, by + bh + 10); glVertex2f(bx - 8, by + bh + 10);
    glEnd();

    /* dome: half-disc with cosine-shaded lunes + observation slit           */
    float dy = by + bh + 10;
    for (int i = 0; i < 10; ++i) {
        float a0 = PI * i / 10, a1 = PI * (i + 1) / 10;
        float shade = 0.50f + 0.50f * sinf((a0 + a1) * 0.5f);
        glColor3f(0.36f * shade + 0.10f, 0.42f * shade + 0.12f, 0.58f * shade + 0.16f);
        glBegin(GL_TRIANGLES);
        glVertex2f(cxm, dy);
        glVertex2f(cxm + cosf(a0) * domeR, dy + sinf(a0) * domeR);
        glVertex2f(cxm + cosf(a1) * domeR, dy + sinf(a1) * domeR);
        glEnd();
    }
    glColor3f(0.10f, 0.13f, 0.24f);                /* dome slit              */
    glBegin(GL_QUADS);
    glVertex2f(cxm - 10, dy + 8); glVertex2f(cxm + 10, dy + 8);
    glVertex2f(cxm + 7, dy + domeR - 4); glVertex2f(cxm - 7, dy + domeR - 4);
    glEnd();

    /* telescope tube poking through the slit toward the sky                 */
    glPushMatrix();
    glTranslatef(cxm, dy + 30, 0);
    glRotatef(58, 0, 0, 1);
    glColor3f(0.14f, 0.17f, 0.30f);
    glBegin(GL_QUADS);
    glVertex2f(-9, 0); glVertex2f(9, 0); glVertex2f(9, 96); glVertex2f(-9, 96);
    glEnd();
    glColor3f(0.42f, 0.75f, 0.95f);                /* glinting lens          */
    glBegin(GL_QUADS);
    glVertex2f(-9, 92); glVertex2f(9, 92); glVertex2f(9, 96); glVertex2f(-9, 96);
    glEnd();
    glPopMatrix();

    /* two warm windows that breathe, plus a door with a small lamp          */
    float pulse = 0.75f + 0.25f * sinf(now * 2.2f);
    glColor4f(1.0f, 0.82f, 0.38f, pulse);
    drawRoundRect(bx + 22, by + 46, 34, 30, 6);
    drawRoundRect(bx + bw - 56, by + 46, 34, 30, 6);
    glColor4f(1.0f, 0.82f, 0.38f, pulse * 0.25f);  /* window glow            */
    drawRoundRect(bx + 16, by + 40, 46, 42, 8);
    drawRoundRect(bx + bw - 62, by + 40, 46, 42, 8);
    glColor3f(0.12f, 0.15f, 0.27f);
    drawRoundRect(cxm - 20, by, 40, 58, 8);        /* door                   */
    glColor4f(1.0f, 0.9f, 0.6f, pulse);
    drawCircle(cxm, by + 66, 4, 12);               /* door lamp              */

    /* radio mast with blinking beacon                                       */
    glColor3f(0.25f, 0.30f, 0.45f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glVertex2f(bx - 26, by); glVertex2f(bx - 26, by + 150);
    glVertex2f(bx - 40, by + 116); glVertex2f(bx - 12, by + 116);
    glVertex2f(bx - 36, by + 134); glVertex2f(bx - 16, by + 134);
    glEnd();
    glLineWidth(1);
    float blink = (sinf(now * 3.0f) > 0.2f) ? 0.95f : 0.15f;
    glColor4f(1.0f, 0.25f, 0.3f, blink);
    drawCircle(bx - 26, by + 154, 5, 12);
}

static void drawPine(float x, float y, float s)
{
    dropShadow(x + 6, y - 2, 34 * s, 7 * s);
    glColor3f(0.28f, 0.19f, 0.13f);                       /* trunk           */
    glBegin(GL_QUADS);
    glVertex2f(x - 5 * s, y); glVertex2f(x + 5 * s, y);
    glVertex2f(x + 4 * s, y + 18 * s); glVertex2f(x - 4 * s, y + 18 * s);
    glEnd();
    for (int t = 0; t < 3; ++t) {                         /* three tiers     */
        float ty = y + (14 + t * 26) * s, w = (46 - t * 10) * s, h = 34 * s;
        glColor3f(0.10f, 0.34f + t * 0.03f, 0.26f);
        glBegin(GL_TRIANGLES);
        glVertex2f(x - w, ty); glVertex2f(x + w, ty); glVertex2f(x, ty + h);
        glEnd();
        glColor4f(0.92f, 0.96f, 1.0f, 0.9f);              /* snow on tier    */
        glBegin(GL_TRIANGLES);
        glVertex2f(x - w * 0.55f, ty + h * 0.42f);
        glVertex2f(x + w * 0.55f, ty + h * 0.42f);
        glVertex2f(x, ty + h);
        glEnd();
    }
}

static void drawSnowman(float x, float y)
{
    dropShadow(x + 6, y - 2, 34, 8);
    float r[3] = { 24, 17, 11 };
    float cy = y + r[0];
    for (int i = 0; i < 3; ++i) {
        glColor3f(0.97f, 0.98f, 1.0f); drawCircle(x, cy, r[i], 36);
        glColor4f(0.55f, 0.65f, 0.85f, 0.35f);            /* right shading   */
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(x + r[i] * 0.35f, cy);
        for (int k = 0; k <= 24; ++k) {
            float a = -PI / 2 + k * PI / 24;
            glVertex2f(x + cosf(a) * r[i] * 0.95f, cy + sinf(a) * r[i] * 0.95f);
        }
        glEnd();
        if (i < 2) cy += r[i] + r[i + 1] - 5;
    }
    float hy = cy;                                        /* head centre     */
    glColor3f(0.08f, 0.08f, 0.1f);                        /* eyes            */
    drawCircle(x - 4, hy + 3, 1.8f, 10); drawCircle(x + 4, hy + 3, 1.8f, 10);
    glColor3f(0.95f, 0.55f, 0.15f);                       /* carrot nose     */
    glBegin(GL_TRIANGLES);
    glVertex2f(x, hy); glVertex2f(x + 1, hy - 3); glVertex2f(x + 14, hy - 4);
    glEnd();
    glColor3f(0.75f, 0.15f, 0.2f);                        /* scarf           */
    drawRoundRect(x - 13, hy - 13, 26, 8, 3);
    drawRoundRect(x + 2, hy - 26, 8, 16, 3);
    glColor3f(0.1f, 0.1f, 0.12f);                         /* top hat         */
    drawRoundRect(x - 14, hy + 8, 28, 5, 2);
    drawRoundRect(x - 9, hy + 11, 18, 16, 2);
    glLineWidth(2);                                       /* stick arms      */
    glColor3f(0.32f, 0.22f, 0.15f);
    glBegin(GL_LINES);
    glVertex2f(x - 20, y + 34); glVertex2f(x - 40, y + 48);
    glVertex2f(x + 20, y + 34); glVertex2f(x + 40, y + 50);
    glEnd();
    glLineWidth(1);
}

/* The engraved wooden signpost - this is where the author's name lives
   inside the scene, as required by the brief.                               */
static void drawSignpost()
{
    float x = 596, y = 186;
    dropShadow(x + 44, y - 3, 60, 8);
    glColor3f(0.36f, 0.25f, 0.16f);                       /* post            */
    glBegin(GL_QUADS);
    glVertex2f(x + 38, y); glVertex2f(x + 50, y);
    glVertex2f(x + 50, y + 92); glVertex2f(x + 38, y + 92);
    glEnd();
    glColor3f(0.47f, 0.33f, 0.21f);                       /* board           */
    drawRoundRect(x - 30, y + 58, 150, 46, 7);
    glColor3f(0.30f, 0.20f, 0.13f);
    drawRoundRect(x - 30, y + 58, 150, 46, 7, false);
    glColor4f(0.30f, 0.20f, 0.13f, 0.9f);                 /* nails           */
    drawCircle(x - 21, y + 96, 2, 8); drawCircle(x + 111, y + 96, 2, 8);

    /* engraved effect: dark text offset one pixel under a bright copy       */
    glColor4f(0.16f, 0.10f, 0.06f, 0.95f);
    strokeCentered(x + 45 + 1, y + 82 - 1, 0.115f, 2.2f, "SIDDHANT DANE");
    glColor4f(0.98f, 0.92f, 0.75f, 0.95f);
    strokeCentered(x + 45, y + 82, 0.115f, 2.2f, "SIDDHANT DANE");
    glColor4f(0.92f, 0.86f, 0.70f, 0.9f);
    textCentered(x + 45, y + 64, GLUT_BITMAP_HELVETICA_10, "AURORA STATION  -  78 N");
}

static void drawSnowfall()
{
    glColor4f(0.95f, 0.97f, 1.0f, 0.85f);
    for (size_t i = 0; i < snowflakes.size(); ++i) {
        const Flake& f = snowflakes[i];
        float sway = sinf(now * 1.3f + f.phase) * f.sway;
        glColor4f(0.95f, 0.97f, 1.0f, 0.35f + 0.5f * (f.r - 1.2f) / 2.4f);
        drawCircle(f.x + sway, f.y, f.r, 8);
    }
}

/* One call renders the whole environment for the current level.             */
static void drawEnvironment()
{
    const Palette& p = PAL[level];
    drawSky(p);
    drawStarfield(p);
    if (p.sun) drawDawnSun();
    drawAurora(p);
    if (!p.sun) drawMoon();
    drawMountains(p);
    drawGround(p);
    drawObservatory();
    drawPine(84, 205, 1.05f);
    drawPine(492, 226, 0.7f);
    drawSnowman(530, 176);
    drawSignpost();
    drawSnowfall();
}

/* ----------------------------------------------------------------------------
   6. THE PUZZLE PLATES
   ----------------------------------------------------------------------------
   Each plate is a miniature picture drawn with primitives inside the frame.
   The reveal trick: the FULL picture is drawn once per revealed tile with
   glScissor clipping to that tile's rectangle, so recovered pieces always
   line up perfectly - exactly like a jigsaw coming together.                */

static void scissorLogical(float x, float y, float w, float h)
{
    /* convert logical design coords to real window pixels for glScissor     */
    glScissor((GLint)(x / LW * winW), (GLint)(y / LH * winH),
              (GLsizei)(w / LW * winW) + 1, (GLsizei)(h / LH * winH) + 1);
}

/* Plate I: "The Great Bear over the icefjord".                              */
static void drawPlate1(float x, float y, float w, float h)
{
    glBegin(GL_QUADS);                                    /* deep night sky  */
    glColor3f(0.03f, 0.05f, 0.17f); glVertex2f(x, y + h); glVertex2f(x + w, y + h);
    glColor3f(0.09f, 0.16f, 0.36f); glVertex2f(x + w, y + 0.35f * h); glVertex2f(x, y + 0.35f * h);
    glEnd();
    glBegin(GL_QUAD_STRIP);                               /* aurora swirl    */
    for (int i = 0; i <= 12; ++i) {
        float fx = x + w * i / 12.0f;
        float fy = y + h * 0.66f + sinf(i * 0.8f + 1.2f) * h * 0.05f;
        glColor4f(0.15f, 0.9f, 0.55f, 0.4f); glVertex2f(fx, fy);
        glColor4f(0.5f, 0.3f, 0.95f, 0.0f);  glVertex2f(fx, fy + h * 0.22f);
    }
    glEnd();
    /* The Big Dipper: seven principal stars with join lines                  */
    const float bd[7][2] = { {0.20f,0.83f},{0.30f,0.86f},{0.40f,0.84f},{0.49f,0.78f},
                             {0.62f,0.79f},{0.63f,0.68f},{0.50f,0.66f} };
    glColor4f(0.6f, 0.8f, 1.0f, 0.5f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < 7; ++i) glVertex2f(x + bd[i][0] * w, y + bd[i][1] * h);
    glVertex2f(x + bd[3][0] * w, y + bd[3][1] * h);
    glEnd();
    glLineWidth(1);
    for (int i = 0; i < 7; ++i) {
        float sx = x + bd[i][0] * w, sy = y + bd[i][1] * h;
        glColor4f(1, 1, 1, 0.95f); drawCircle(sx, sy, 3.4f, 12);
        glColor4f(0.7f, 0.85f, 1.0f, 0.30f); drawCircle(sx, sy, 7, 12);
    }
    glColor3f(0.06f, 0.10f, 0.24f);                       /* fjord ridge     */
    glBegin(GL_TRIANGLE_STRIP);
    glVertex2f(x, y + 0.34f * h);          glVertex2f(x, y + 0.52f * h);
    glVertex2f(x + 0.28f * w, y + 0.34f*h);glVertex2f(x + 0.34f * w, y + 0.58f * h);
    glVertex2f(x + 0.55f * w, y + 0.34f*h);glVertex2f(x + 0.72f * w, y + 0.62f * h);
    glVertex2f(x + w, y + 0.34f * h);      glVertex2f(x + w, y + 0.48f * h);
    glEnd();
    glBegin(GL_QUADS);                                    /* still water     */
    glColor3f(0.09f, 0.16f, 0.36f); glVertex2f(x, y);         glVertex2f(x + w, y);
    glColor3f(0.16f, 0.28f, 0.55f); glVertex2f(x + w, y + 0.34f * h); glVertex2f(x, y + 0.34f * h);
    glEnd();
    /* the fjord mirrors the lights: aurora bands, a moon glade, star glints */
    for (int b = 0; b < 3; ++b) {
        glColor4f(0.25f, 0.95f, 0.6f, 0.30f - b * 0.08f);
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= 10; ++i) {
            float fx = x + w * i / 10.0f;
            float fy = y + (0.26f - b * 0.085f) * h + sinf(i * 1.1f + b * 2) * 0.012f * h;
            glVertex2f(fx, fy);
            glVertex2f(fx, fy - (0.045f - b * 0.01f) * h);
        }
        glEnd();
    }
    glColor4f(0.85f, 0.9f, 1.0f, 0.20f);                  /* moon glade      */
    glBegin(GL_QUADS);
    glVertex2f(x + 0.10f * w, y + 0.02f * h); glVertex2f(x + 0.20f * w, y + 0.02f * h);
    glVertex2f(x + 0.17f * w, y + 0.33f * h); glVertex2f(x + 0.13f * w, y + 0.33f * h);
    glEnd();
    for (int g = 0; g < 8; ++g) {                         /* star glints     */
        glColor4f(0.8f, 0.9f, 1.0f, 0.5f);
        drawCircle(x + (0.28f + 0.085f * g) * w, y + (0.06f + 0.028f * (g % 4)) * h, 1.5f, 8);
    }
    /* a small crescent moon tucked in the plate's top corner                */
    glColor4f(0.93f, 0.95f, 1.0f, 0.95f); drawCircle(x + 0.11f * w, y + 0.90f * h, 0.045f * w, 30);
    glColor4f(0.05f, 0.08f, 0.22f, 1.0f); drawCircle(x + 0.13f * w, y + 0.915f * h, 0.040f * w, 30);
    glColor4f(0.85f, 0.92f, 1.0f, 0.85f);
    textCentered(x + w * 0.5f, y + 0.045f * h, GLUT_BITMAP_HELVETICA_12, "PLATE I  -  URSA MAJOR");
}

/* Plate II: "Midnight sun bears" - mother and cub on an ice floe.           */
static void drawPlate2(float x, float y, float w, float h)
{
    glBegin(GL_QUADS);                                    /* rose dawn sky   */
    glColor3f(0.30f, 0.16f, 0.40f); glVertex2f(x, y + h); glVertex2f(x + w, y + h);
    glColor3f(0.99f, 0.62f, 0.45f); glVertex2f(x + w, y + 0.42f * h); glVertex2f(x, y + 0.42f * h);
    glEnd();
    float scx = x + 0.5f * w, scy = y + 0.46f * h;        /* low sun         */
    for (int i = 4; i >= 1; --i) { glColor4f(1, 0.8f, 0.5f, 0.08f); drawCircle(scx, scy, 0.06f * w + i * 0.045f * w); }
    glColor3f(1.0f, 0.88f, 0.6f); drawCircle(scx, scy, 0.062f * w, 36);
    glBegin(GL_QUADS);                                    /* sea             */
    glColor3f(0.55f, 0.30f, 0.45f); glVertex2f(x, y);          glVertex2f(x + w, y);
    glColor3f(0.95f, 0.55f, 0.50f); glVertex2f(x + w, y + 0.42f * h); glVertex2f(x, y + 0.42f * h);
    glEnd();
    glColor4f(1.0f, 0.85f, 0.6f, 0.5f);                   /* sun glitter     */
    glBegin(GL_QUADS);
    glVertex2f(scx - 0.05f * w, y + 0.1f * h); glVertex2f(scx + 0.05f * w, y + 0.1f * h);
    glVertex2f(scx + 0.03f * w, y + 0.40f * h); glVertex2f(scx - 0.03f * w, y + 0.40f * h);
    glEnd();
    glColor3f(0.93f, 0.95f, 1.0f);                        /* ice floe        */
    glBegin(GL_POLYGON);
    glVertex2f(x + 0.12f * w, y + 0.20f * h); glVertex2f(x + 0.30f * w, y + 0.27f * h);
    glVertex2f(x + 0.62f * w, y + 0.28f * h); glVertex2f(x + 0.88f * w, y + 0.22f * h);
    glVertex2f(x + 0.80f * w, y + 0.13f * h); glVertex2f(x + 0.22f * w, y + 0.12f * h);
    glEnd();
    glColor4f(0.65f, 0.75f, 0.95f, 0.6f);                 /* floe waterline  */
    glBegin(GL_QUADS);
    glVertex2f(x + 0.18f * w, y + 0.115f * h); glVertex2f(x + 0.82f * w, y + 0.115f * h);
    glVertex2f(x + 0.80f * w, y + 0.095f * h); glVertex2f(x + 0.20f * w, y + 0.095f * h);
    glEnd();

    /* mother bear: ellipse body, round head, stub legs, all primitives      */
    float bx = x + 0.42f * w, by = y + 0.27f * h;
    glColor3f(0.98f, 0.97f, 0.94f);
    drawEllipse(bx, by + 0.075f * h, 0.115f * w, 0.075f * h);
    drawCircle(bx + 0.105f * w, by + 0.115f * h, 0.045f * w, 24);          /* head */
    drawCircle(bx + 0.135f * w, by + 0.150f * h, 0.013f * w, 12);          /* ear  */
    drawCircle(bx + 0.085f * w, by + 0.152f * h, 0.013f * w, 12);
    glBegin(GL_QUADS);                                                     /* legs */
    for (int l = 0; l < 3; ++l) {
        float lx = bx - 0.08f * w + l * 0.07f * w;
        glVertex2f(lx, by + 0.02f * h); glVertex2f(lx + 0.028f * w, by + 0.02f * h);
        glVertex2f(lx + 0.028f * w, by - 0.015f * h); glVertex2f(lx, by - 0.015f * h);
    }
    glEnd();
    glColor4f(0.75f, 0.72f, 0.78f, 0.5f);                 /* belly shading   */
    drawEllipse(bx - 0.02f * w, by + 0.05f * h, 0.085f * w, 0.04f * h);
    glColor3f(0.1f, 0.1f, 0.12f);
    drawCircle(bx + 0.146f * w, by + 0.117f * h, 0.008f * w, 10);          /* nose */
    drawCircle(bx + 0.112f * w, by + 0.127f * h, 0.005f * w, 8);           /* eye  */

    /* cub                                                                    */
    float ux = x + 0.66f * w, uy = y + 0.245f * h;
    glColor3f(0.98f, 0.97f, 0.94f);
    drawEllipse(ux, uy + 0.045f * h, 0.06f * w, 0.042f * h);
    drawCircle(ux + 0.055f * w, uy + 0.07f * h, 0.026f * w, 20);
    drawCircle(ux + 0.070f * w, uy + 0.09f * h, 0.009f * w, 10);
    drawCircle(ux + 0.042f * w, uy + 0.092f * h, 0.009f * w, 10);
    glColor3f(0.1f, 0.1f, 0.12f);
    drawCircle(ux + 0.077f * w, uy + 0.071f * h, 0.005f * w, 8);
    drawCircle(ux + 0.057f * w, uy + 0.078f * h, 0.004f * w, 8);

    glColor4f(0.25f, 0.12f, 0.25f, 0.85f);                /* distant terns   */
    glLineWidth(2);
    glBegin(GL_LINES);
    for (int b = 0; b < 3; ++b) {
        float tx = x + (0.18f + b * 0.1f) * w, ty = y + (0.80f - b * 0.05f) * h;
        glVertex2f(tx - 0.02f * w, ty); glVertex2f(tx, ty + 0.015f * h);
        glVertex2f(tx, ty + 0.015f * h); glVertex2f(tx + 0.02f * w, ty);
    }
    glEnd();
    glLineWidth(1);
    glColor4f(0.35f, 0.15f, 0.25f, 0.9f);
    textCentered(x + w * 0.5f, y + 0.045f * h, GLUT_BITMAP_HELVETICA_12, "PLATE II  -  MIDNIGHT SUN");
}

/* Frame, frosted tiles and scissor-based reveal.                            */
static void drawPuzzle(bool forceComplete)
{
    float fx = PZ_X - PZ_BORDER, fy = PZ_Y - PZ_BORDER;
    float fw = PZ_W + 2 * PZ_BORDER, fh = PZ_H + 2 * PZ_BORDER;

    dropShadow(PZ_X + PZ_W / 2 + 10, PZ_Y - 22, PZ_W * 0.52f, 14);
    glColor3f(0.78f, 0.88f, 0.97f);                       /* ice frame       */
    drawRoundRect(fx, fy, fw, fh, 16);
    glColor3f(0.55f, 0.70f, 0.88f);
    drawRoundRect(fx, fy, fw, fh, 16, false);
    glColor4f(1, 1, 1, 0.5f);                             /* frame highlight */
    glLineWidth(2);
    glBegin(GL_LINES);
    glVertex2f(fx + 10, fy + fh - 6); glVertex2f(fx + fw * 0.45f, fy + fh - 6);
    glEnd();
    glLineWidth(1);
    glColor3f(0.06f, 0.09f, 0.2f);                        /* dark backing    */
    glBegin(GL_QUADS);
    glVertex2f(PZ_X, PZ_Y); glVertex2f(PZ_X + PZ_W, PZ_Y);
    glVertex2f(PZ_X + PZ_W, PZ_Y + PZ_H); glVertex2f(PZ_X, PZ_Y + PZ_H);
    glEnd();

    float tw = PZ_W / 3.0f, th = PZ_H / 2.0f;
    int show = forceComplete ? TILES : revealed;

    glEnable(GL_SCISSOR_TEST);
    for (int i = 0; i < show; ++i) {
        int col = i % 3, row = i / 3;
        float tx = PZ_X + col * tw, ty = PZ_Y + row * th;
        scissorLogical(tx, ty, tw, th);
        if (level == 0) drawPlate1(PZ_X, PZ_Y, PZ_W, PZ_H);
        else            drawPlate2(PZ_X, PZ_Y, PZ_W, PZ_H);
        /* fade-in veil while the piece "develops"                           */
        float age = now - tileReveal[i];
        if (!forceComplete && age < 0.7f) {
            glColor4f(0.85f, 0.95f, 1.0f, 1.0f - age / 0.7f);
            glBegin(GL_QUADS);
            glVertex2f(tx, ty); glVertex2f(tx + tw, ty);
            glVertex2f(tx + tw, ty + th); glVertex2f(tx, ty + th);
            glEnd();
        }
    }
    glDisable(GL_SCISSOR_TEST);

    /* frosted missing pieces with bevelled edges and a snowflake stamp      */
    for (int i = show; i < TILES; ++i) {
        int col = i % 3, row = i / 3;
        float tx = PZ_X + col * tw, ty = PZ_Y + row * th;
        glColor4f(0.72f, 0.82f, 0.93f, 0.96f);
        glBegin(GL_QUADS);
        glVertex2f(tx + 2, ty + 2); glVertex2f(tx + tw - 2, ty + 2);
        glVertex2f(tx + tw - 2, ty + th - 2); glVertex2f(tx + 2, ty + th - 2);
        glEnd();
        glLineWidth(2);                                   /* bevel: light TL */
        glColor4f(0.95f, 0.99f, 1.0f, 0.9f);
        glBegin(GL_LINE_STRIP);
        glVertex2f(tx + 3, ty + 3); glVertex2f(tx + 3, ty + th - 3); glVertex2f(tx + tw - 3, ty + th - 3);
        glEnd();
        glColor4f(0.45f, 0.55f, 0.75f, 0.9f);             /* bevel: dark BR  */
        glBegin(GL_LINE_STRIP);
        glVertex2f(tx + 3, ty + 3); glVertex2f(tx + tw - 3, ty + 3); glVertex2f(tx + tw - 3, ty + th - 3);
        glEnd();
        glLineWidth(1);
        snowflakeIcon(tx + tw / 2, ty + th / 2 + 8, 14, true);
        glColor4f(0.3f, 0.4f, 0.6f, 0.9f);
        textCentered(tx + tw / 2, ty + th / 2 - 26, GLUT_BITMAP_HELVETICA_18, "?");
    }

    /* thin grid so recovered pieces still read as pieces                    */
    glColor4f(0.85f, 0.92f, 1.0f, 0.35f);
    glBegin(GL_LINES);
    for (int c = 1; c < 3; ++c) { glVertex2f(PZ_X + c * tw, PZ_Y); glVertex2f(PZ_X + c * tw, PZ_Y + PZ_H); }
    glVertex2f(PZ_X, PZ_Y + th); glVertex2f(PZ_X + PZ_W, PZ_Y + th);
    glEnd();

    char cap[64];
    snprintf(cap, sizeof cap, "TELESCOPE ARCHIVE  -  PIECES %d / %d", show, TILES);
    glColor4f(0.85f, 0.92f, 1.0f, 0.95f);
    textCentered(PZ_X + PZ_W / 2, PZ_Y + PZ_H + PZ_BORDER + 10, GLUT_BITMAP_HELVETICA_12, cap);
}

/* ----------------------------------------------------------------------------
   7. HUD AND PANELS
   --------------------------------------------------------------------------*/

static void drawHUD()
{
    const Palette& p = PAL[level];
    glColor4f(0.02f, 0.04f, 0.12f, 0.55f);                /* top bar         */
    glBegin(GL_QUADS);
    glVertex2f(0, 658); glVertex2f(LW, 658); glVertex2f(LW, LH); glVertex2f(0, LH);
    glEnd();
    glColor4f(p.aurA[0], p.aurA[1], p.aurA[2], 0.8f);
    glLineWidth(2);
    glBegin(GL_LINES); glVertex2f(0, 658); glVertex2f(LW, 658); glEnd();
    glLineWidth(1);

    char buf[96];
    snprintf(buf, sizeof buf, "LEVEL %d  -  %s", level + 1, PAL[level].name);
    glColor3f(0.85f, 0.93f, 1.0f);
    text(24, 694, GLUT_BITMAP_HELVETICA_18, buf);
    glColor4f(0.65f, 0.78f, 0.95f, 0.9f);
    text(24, 672, GLUT_BITMAP_HELVETICA_12, "OBSERVER: SIDDHANT DANE");

    snprintf(buf, sizeof buf, "SCORE  %d", score);
    glColor3f(1.0f, 0.92f, 0.6f);
    textCentered(590, 694, GLUT_BITMAP_HELVETICA_18, buf);
    if (streak >= 2) {
        snprintf(buf, sizeof buf, "STREAK x%d  (+%d bonus)", streak, 25 * (streak - 1) > 100 ? 100 : 25 * (streak - 1));
        glColor3f(0.55f, 1.0f, 0.75f);
        textCentered(590, 672, GLUT_BITMAP_HELVETICA_12, buf);
    }

    glColor4f(0.65f, 0.78f, 0.95f, 0.9f);                 /* heat cells      */
    text(1064, 694, GLUT_BITMAP_HELVETICA_12, "HEAT");
    for (int i = 0; i < 3; ++i) snowflakeIcon(1120 + i * 44, 690, 13, i < lives);

    if (state == S_PLAYING && !paused) {                  /* question timer  */
        float f = clampf(timeLeft / Q_TIME, 0, 1);
        glColor4f(0.1f, 0.12f, 0.25f, 0.8f);
        drawRoundRect(340, 664, 500, 9, 4);
        if (f > 0.02f) {
            glColor3f(lerpf(0.95f, 0.2f, f), lerpf(0.25f, 0.9f, f), 0.35f);
            drawRoundRect(340, 664, 500 * f, 9, 4);
        }
    }
}

static void drawKeyChip(float x, float y, const char* k, bool highlight, bool good)
{
    if (highlight) {
        if (good) glColor4f(0.15f, 0.65f, 0.35f, 0.95f);
        else      glColor4f(0.75f, 0.20f, 0.25f, 0.95f);
    } else        glColor4f(0.16f, 0.22f, 0.42f, 0.95f);
    drawRoundRect(x, y, 26, 24, 6);
    glColor4f(0.7f, 0.85f, 1.0f, 0.9f);
    drawRoundRect(x, y, 26, 24, 6, false);
    glColor3f(0.92f, 0.96f, 1.0f);
    textCentered(x + 13, y + 6, GLUT_BITMAP_HELVETICA_12, k);
}

static void drawQuestionPanel()
{
    /* while giving feedback the answered question may already have left the
       queue, so it is remembered in fbQIdx                                  */
    if (state == S_PLAYING && queueQ.empty()) return;
    const Question& q = BANK[level][state == S_FEEDBACK ? fbQIdx : queueQ.front()];

    glColor4f(0.03f, 0.05f, 0.14f, 0.68f);
    drawRoundRect(18, 10, LW - 36, 150, 14);
    glColor4f(0.55f, 0.75f, 0.95f, 0.7f);
    drawRoundRect(18, 10, LW - 36, 150, 14, false);

    glColor3f(0.95f, 0.97f, 1.0f);
    text(44, 130, GLUT_BITMAP_HELVETICA_18, q.text);

    if (state == S_PLAYING) {
        for (int i = 0; i < 3; ++i) {
            char key[2] = { (char)('1' + i), 0 };
            drawKeyChip(44, 88 - i * 34, key, false, false);
            glColor3f(0.85f, 0.90f, 1.0f);
            text(84, 94 - i * 34, GLUT_BITMAP_HELVETICA_18, q.opt[i]);
        }
        glColor4f(0.55f, 0.68f, 0.9f, 0.8f);
        text(LW - 300, 24, GLUT_BITMAP_HELVETICA_12, "[1-3] answer   [P] pause   [ESC] quit");
    }
    else if (state == S_FEEDBACK) {
        /* show the outcome, the right answer and a fact                     */
        if (fbCorrect) {
            char msg[64];
            snprintf(msg, sizeof msg, "CORRECT!  +%d points  -  piece recovered", fbGained);
            glColor3f(0.5f, 1.0f, 0.7f);
            text(44, 96, GLUT_BITMAP_HELVETICA_18, msg);
        } else {
            glColor3f(1.0f, 0.55f, 0.55f);
            text(44, 96, GLUT_BITMAP_HELVETICA_18,
                 fbTimeout ? "OUT OF TIME!  The cold creeps in..." : "NOT QUITE...  the cold creeps in");
            char msg[160];
            snprintf(msg, sizeof msg, "Correct answer: %s", q.opt[q.correct]);
            glColor3f(0.95f, 0.9f, 0.65f);
            text(44, 68, GLUT_BITMAP_HELVETICA_12, msg);
        }
        glColor4f(0.75f, 0.88f, 1.0f, 0.95f);
        char factLine[220];
        snprintf(factLine, sizeof factLine, "Field note: %s", q.fact);
        text(44, fbCorrect ? 62 : 44, GLUT_BITMAP_HELVETICA_12, factLine);
        if (now - fbStart > 0.35f) {
            float blink = 0.6f + 0.4f * sinf(now * 4);
            glColor4f(0.95f, 0.97f, 1.0f, blink);
            text(LW - 260, 24, GLUT_BITMAP_HELVETICA_12, "[ENTER] continue");
        }
    }
}

/* Edge flash after each answer: green sweep for correct, red for wrong.     */
static void drawFlash()
{
    float age = now - flashStart;
    if (age > 0.6f) return;
    float a = (1.0f - age / 0.6f) * 0.42f;
    if (flashGood) glColor4f(0.2f, 0.95f, 0.5f, a);
    else           glColor4f(0.95f, 0.2f, 0.25f, a);
    float t = 46;
    glBegin(GL_QUADS);                                    /* four edge bands */
    glVertex2f(0, 0); glVertex2f(LW, 0); glVertex2f(LW, t); glVertex2f(0, t);
    glVertex2f(0, LH - t); glVertex2f(LW, LH - t); glVertex2f(LW, LH); glVertex2f(0, LH);
    glVertex2f(0, 0); glVertex2f(t, 0); glVertex2f(t, LH); glVertex2f(0, LH);
    glVertex2f(LW - t, 0); glVertex2f(LW, 0); glVertex2f(LW, LH); glVertex2f(LW - t, LH);
    glEnd();
}

static void dimScreen(float a)
{
    glColor4f(0.01f, 0.02f, 0.08f, a);
    glBegin(GL_QUADS);
    glVertex2f(0, 0); glVertex2f(LW, 0); glVertex2f(LW, LH); glVertex2f(0, LH);
    glEnd();
}

static void drawMenu()
{
    dimScreen(0.35f);
    glColor4f(0.03f, 0.05f, 0.14f, 0.72f);
    drawRoundRect(310, 150, 660, 430, 18);
    glColor4f(0.55f, 0.75f, 0.95f, 0.8f);
    drawRoundRect(310, 150, 660, 430, 18, false);

    /* layered title: icy shadow + bright face                               */
    glColor4f(0.1f, 0.5f, 0.6f, 0.9f);
    strokeCentered(642, 496, 0.5f, 5.0f, "POLAR SKIES");
    glColor4f(0.75f, 0.98f, 0.95f, 1.0f);
    strokeCentered(640, 500, 0.5f, 5.0f, "POLAR SKIES");
    glColor3f(0.95f, 0.9f, 0.65f);
    textCentered(640, 452, GLUT_BITMAP_HELVETICA_18, "An Arctic Observatory Puzzle Quiz");
    glColor4f(0.7f, 0.82f, 1.0f, 0.9f);
    textCentered(640, 424, GLUT_BITMAP_HELVETICA_12, "designed & programmed by SIDDHANT DANE");

    glColor4f(0.85f, 0.92f, 1.0f, 0.95f);
    textCentered(640, 372, GLUT_BITMAP_HELVETICA_12, "A storm shattered the station's telescope archive.");
    textCentered(640, 350, GLUT_BITMAP_HELVETICA_12, "Answer polar-science questions to recover the plates piece by piece.");

    float yy = 296;
    const char* lines[4] = { "[1] [2] [3]   choose an answer",
                             "[ENTER]       start / continue",
                             "[P] pause     [F9] screenshot",
                             "[R] restart   [ESC] quit" };
    for (int i = 0; i < 4; ++i) {
        glColor4f(0.75f, 0.88f, 1.0f, 0.95f);
        textCentered(640, yy - i * 26, GLUT_BITMAP_9_BY_15, lines[i]);
    }
    float blink = 0.55f + 0.45f * sinf(now * 3.2f);
    glColor4f(0.55f, 1.0f, 0.8f, blink);
    textCentered(640, 178, GLUT_BITMAP_HELVETICA_18, "Press ENTER to take the night shift");
}

static void drawLevelDone()
{
    dimScreen(0.45f);
    glColor4f(0.03f, 0.06f, 0.16f, 0.8f);
    drawRoundRect(340, 210, 600, 320, 18);
    glColor4f(0.3f, 0.9f, 0.6f, 0.8f);
    drawRoundRect(340, 210, 600, 320, 18, false);
    glColor4f(0.6f, 1.0f, 0.8f, 1.0f);
    strokeCentered(640, 470, 0.24f, 3.0f, "PLATE RESTORED");
    char buf[96];
    glColor3f(0.9f, 0.95f, 1.0f);
    snprintf(buf, sizeof buf, "The %s archive is complete.", level == 0 ? "Great Bear" : "Midnight Sun");
    textCentered(640, 424, GLUT_BITMAP_HELVETICA_18, buf);
    snprintf(buf, sizeof buf, "Score so far: %d      Best streak: x%d      Misses: %d", score, bestStreak, wrongCount);
    glColor3f(0.95f, 0.9f, 0.65f);
    textCentered(640, 372, GLUT_BITMAP_HELVETICA_12, buf);
    glColor4f(0.75f, 0.88f, 1.0f, 0.95f);
    textCentered(640, 330, GLUT_BITMAP_HELVETICA_12, "Dawn is breaking over the fjord. A second plate awaits");
    textCentered(640, 308, GLUT_BITMAP_HELVETICA_12, "at the morning station.");
    float blink = 0.55f + 0.45f * sinf(now * 3.2f);
    glColor4f(0.55f, 1.0f, 0.8f, blink);
    textCentered(640, 244, GLUT_BITMAP_HELVETICA_18, "Press ENTER to travel on");
}

static void drawGameDone()
{
    dimScreen(0.5f);
    /* celebratory star bursts cycling outward                               */
    for (int b = 0; b < 3; ++b) {
        float cyc = fmodf(now * 0.6f + b * 0.33f, 1.0f);
        float cx = 320 + b * 320, cy = 520 + 60 * sinf(b * 2.1f);
        for (int i = 0; i < 10; ++i) {
            float a = i * 2 * PI / 10 + b;
            glColor4f(0.6f + 0.4f * sinf(b + i), 0.9f, 1.0f - 0.4f * cyc, (1 - cyc) * 0.8f);
            drawCircle(cx + cosf(a) * cyc * 110, cy + sinf(a) * cyc * 110, 3.5f * (1 - cyc) + 1, 8);
        }
    }
    glColor4f(0.03f, 0.06f, 0.16f, 0.8f);
    drawRoundRect(320, 180, 640, 330, 18);
    glColor4f(1.0f, 0.85f, 0.4f, 0.85f);
    drawRoundRect(320, 180, 640, 330, 18, false);
    glColor4f(0.4f, 0.85f, 0.65f, 0.9f);
    strokeCentered(642, 446, 0.34f, 4.0f, "EXPEDITION COMPLETE");
    glColor4f(1.0f, 0.95f, 0.75f, 1.0f);
    strokeCentered(640, 450, 0.34f, 4.0f, "EXPEDITION COMPLETE");

    const char* rank = score >= 1500 ? "CHIEF AURORA SCIENTIST" :
                       score >= 1000 ? "SENIOR FIELD RESEARCHER" : "TRAINEE OBSERVER";
    char buf[96];
    snprintf(buf, sizeof buf, "Final score: %d", score);
    glColor3f(0.95f, 0.9f, 0.65f);
    textCentered(640, 396, GLUT_BITMAP_HELVETICA_18, buf);
    snprintf(buf, sizeof buf, "Rank earned: %s", rank);
    glColor3f(0.6f, 1.0f, 0.8f);
    textCentered(640, 362, GLUT_BITMAP_HELVETICA_18, buf);
    glColor4f(0.75f, 0.88f, 1.0f, 0.95f);
    textCentered(640, 316, GLUT_BITMAP_HELVETICA_12, "Both archive plates are restored. The station's records are safe -");
    textCentered(640, 294, GLUT_BITMAP_HELVETICA_12, "and the aurora dances on. Thank you for playing, observer.");
    float blink = 0.55f + 0.45f * sinf(now * 3.2f);
    glColor4f(0.55f, 1.0f, 0.8f, blink);
    textCentered(640, 226, GLUT_BITMAP_HELVETICA_18, "Press R to play again  -  ESC to exit");
}

static void drawGameOver()
{
    dimScreen(0.55f);
    glColor4f(0.14f, 0.03f, 0.07f, 0.85f);
    drawRoundRect(360, 240, 560, 260, 18);
    glColor4f(0.95f, 0.35f, 0.4f, 0.85f);
    drawRoundRect(360, 240, 560, 260, 18, false);
    glColor4f(1.0f, 0.6f, 0.6f, 1.0f);
    strokeCentered(640, 430, 0.30f, 3.6f, "WHITEOUT");
    glColor3f(0.95f, 0.9f, 0.85f);
    textCentered(640, 384, GLUT_BITMAP_HELVETICA_18, "The heat cells are empty and the storm rolls in.");
    glColor4f(0.9f, 0.8f, 0.8f, 0.95f);
    textCentered(640, 344, GLUT_BITMAP_HELVETICA_12, "Every recovered piece of this plate was lost to the wind.");
    float blink = 0.55f + 0.45f * sinf(now * 3.2f);
    glColor4f(1.0f, 0.8f, 0.6f, blink);
    textCentered(640, 286, GLUT_BITMAP_HELVETICA_18, "Press ENTER to retry the level");
}

static void drawPauseOverlay()
{
    dimScreen(0.5f);
    glColor4f(0.7f, 0.9f, 1.0f, 0.95f);
    strokeCentered(640, 380, 0.4f, 4.0f, "PAUSED");
    glColor4f(0.85f, 0.92f, 1.0f, 0.9f);
    textCentered(640, 330, GLUT_BITMAP_HELVETICA_18, "The aurora waits for no one... but the timer does.");
    textCentered(640, 298, GLUT_BITMAP_HELVETICA_12, "Press P to resume");
}

/* ----------------------------------------------------------------------------
   8. GAME LOGIC
   --------------------------------------------------------------------------*/

static void startLevel(int lv)
{
    level = lv;
    levelStartScore = score;
    revealed = 0;
    lives = 3;
    streak = 0;
    queueQ.clear();
    for (int i = 0; i < 6; ++i) queueQ.push_back(i);
    for (int i = 0; i < TILES; ++i) tileReveal[i] = -10;
    timeLeft = Q_TIME;
    state = S_PLAYING;
}

static void resetGame()
{
    score = 0; wrongCount = 0; bestStreak = 0;
    startLevel(0);
}

static void submitAnswer(int choice)   /* choice -1 means the timer expired  */
{
    if (state != S_PLAYING || queueQ.empty()) return;
    fbQIdx = queueQ.front();
    const Question& q = BANK[level][fbQIdx];

    fbChosen  = choice;
    fbTimeout = (choice < 0);
    fbCorrect = (choice == q.correct);
    fbStart   = now;
    flashStart= now;
    flashGood = fbCorrect;
    state     = S_FEEDBACK;

    if (fbCorrect) {
        int bonus = 25 * (streak);                 /* streak built BEFORE +1 */
        if (bonus > 100) bonus = 100;
        fbGained = 100 + bonus;
        score += fbGained;
        streak += 1;
        if (streak > bestStreak) bestStreak = streak;
        tileReveal[revealed] = now;
        revealed += 1;
        queueQ.erase(queueQ.begin());
    } else {
        wrongCount += 1;
        streak = 0;
        score -= 25; if (score < 0) score = 0;
        lives -= 1;
        /* the question goes to the back of the queue and returns later      */
        int idx = queueQ.front();
        queueQ.erase(queueQ.begin());
        queueQ.push_back(idx);
    }
}

static void advanceAfterFeedback()
{
    if (!fbCorrect && lives <= 0) { state = S_GAME_OVER; return; }
    if (revealed >= TILES) {
        state = (level == 0) ? S_LEVEL_DONE : S_GAME_DONE;
        return;
    }
    timeLeft = Q_TIME;
    state = S_PLAYING;
}

/* ----------------------------------------------------------------------------
   9. SCREENSHOT (F9) - dumps the framebuffer to a 24-bit BMP
   ----------------------------------------------------------------------------
   Used to capture the submission figures directly from the running game.    */

static void saveScreenshot()
{
    static int shot = 0;
    int w = winW - (winW % 4);              /* keep rows 4-byte aligned      */
    int h = winH;
    std::vector<unsigned char> px(w * h * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, &px[0]);

    char name[64];
    snprintf(name, sizeof name, "polarskies_shot%02d.bmp", ++shot);
    FILE* f = fopen(name, "wb");
    if (!f) return;

    int rowBytes = w * 3;
    int imgBytes = rowBytes * h;
    unsigned char fileHdr[14] = { 'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0 };
    unsigned char infoHdr[40] = { 40,0,0,0 };
    int fileSize = 54 + imgBytes;
    fileHdr[2] = fileSize & 255; fileHdr[3] = (fileSize >> 8) & 255;
    fileHdr[4] = (fileSize >> 16) & 255; fileHdr[5] = (fileSize >> 24) & 255;
    infoHdr[4] = w & 255; infoHdr[5] = (w >> 8) & 255; infoHdr[6] = (w >> 16) & 255;
    infoHdr[8] = h & 255; infoHdr[9] = (h >> 8) & 255; infoHdr[10] = (h >> 16) & 255;
    infoHdr[12] = 1; infoHdr[14] = 24;
    fwrite(fileHdr, 1, 14, f); fwrite(infoHdr, 1, 40, f);
    for (int yy = 0; yy < h; ++yy)          /* BMP stores BGR bottom-up      */
        for (int xx = 0; xx < w; ++xx) {
            unsigned char* p = &px[(yy * w + xx) * 3];
            fputc(p[2], f); fputc(p[1], f); fputc(p[0], f);
        }
    fclose(f);
    printf("Saved %s\n", name);
}

/* ----------------------------------------------------------------------------
   10. GLUT CALLBACKS
   --------------------------------------------------------------------------*/

static void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glOrtho(0, LW, 0, LH, -1, 1);
    glMatrixMode(GL_MODELVIEW);  glLoadIdentity();

    drawEnvironment();
    drawPuzzle(state == S_LEVEL_DONE || state == S_GAME_DONE);
    drawHUD();

    switch (state) {
        case S_MENU:       drawMenu();                          break;
        case S_PLAYING:
            drawQuestionPanel();
            if (paused) drawPauseOverlay();
            break;
        case S_FEEDBACK:   drawQuestionPanel(); drawFlash();    break;
        case S_LEVEL_DONE: drawLevelDone();                     break;
        case S_GAME_DONE:  drawGameDone();                      break;
        case S_GAME_OVER:  drawGameOver();                      break;
    }
    glutSwapBuffers();
}

static void reshape(int w, int h)
{
    winW = w > 1 ? w : 1;
    winH = h > 1 ? h : 1;
    glViewport(0, 0, winW, winH);
}

static void tick(int)
{
    float t = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float dt = t - lastFrame;
    lastFrame = t;
    now = t;

    if (state == S_PLAYING && !paused) {
        timeLeft -= dt;
        if (timeLeft <= 0) { timeLeft = 0; submitAnswer(-1); }
    }
    for (size_t i = 0; i < snowflakes.size(); ++i) {      /* snow physics    */
        Flake& f = snowflakes[i];
        f.y -= f.v * dt;
        if (f.y < -6) { f.y = LH + frand(4, 60); f.x = frand(0, LW); }
    }
    glutPostRedisplay();
    glutTimerFunc(16, tick, 0);
}

static void keyboard(unsigned char key, int, int)
{
    switch (key) {
        case 27:  exit(0);                                            break;
        case 'p': case 'P':
            if (state == S_PLAYING) paused = !paused;
            break;
        case 'r': case 'R':
            resetGame();                                              break;
        case 13: case ' ':                       /* ENTER / SPACE     */
            if      (state == S_MENU)       resetGame();
            else if (state == S_FEEDBACK && now - fbStart > 0.35f) advanceAfterFeedback();
            else if (state == S_LEVEL_DONE) startLevel(1);
            else if (state == S_GAME_OVER)  startLevel(level);
            break;
        case '1': case '2': case '3':
            if (state == S_PLAYING && !paused) submitAnswer(key - '1');
            break;
    }
}

static void special(int key, int, int)
{
    if (key == GLUT_KEY_F9) saveScreenshot();
}

/* ----------------------------------------------------------------------------
   11. INITIALISATION
   --------------------------------------------------------------------------*/

static void initWorld()
{
    srand(7);                                /* fixed seed = repeatable sky  */
    stars.clear();
    for (int i = 0; i < 130; ++i) {
        Star s;
        s.x = frand(0, LW); s.y = frand(300, LH - 8);
        s.r = frand(0.8f, 2.4f);
        s.speed = frand(1.2f, 3.4f); s.phase = frand(0, 2 * PI);
        stars.push_back(s);
    }
    snowflakes.clear();
    for (int i = 0; i < 150; ++i) {
        Flake f;
        f.x = frand(0, LW); f.y = frand(0, LH);
        f.r = frand(1.2f, 3.6f); f.v = frand(26, 80);
        f.sway = frand(4, 16); f.phase = frand(0, 2 * PI);
        snowflakes.push_back(f);
    }
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1280, 720);
    glutInitWindowPosition(60, 40);
    glutCreateWindow("POLAR SKIES - an Arctic observatory puzzle quiz | Siddhant Dane");

    glClearColor(0.02f, 0.04f, 0.12f, 1);
    glEnable(GL_BLEND);                      /* translucency everywhere      */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glShadeModel(GL_SMOOTH);

    initWorld();
    for (int i = 0; i < TILES; ++i) tileReveal[i] = -10;

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutTimerFunc(16, tick, 0);
    glutMainLoop();
    return 0;
}
