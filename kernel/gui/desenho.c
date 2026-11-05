/* desenho.c - rotinas de desenho com recorte, usadas pela área de trabalho
 * e pelo gerenciador de janelas. Tudo é recortado contra um retângulo, para
 * que redesenhar só um pedaço da tela nunca pinte fora dele. */
#include "desenho.h"

extern const unsigned char fonte_sagui[256][16];

void sup_iniciar(superficie_t *s, uint32_t *px, int larg, int alt)
{
    s->px = px; s->larg = larg; s->alt = alt;
    sup_recorte(s, 0, 0, 0, 0);
}

void sup_recorte(superficie_t *s, int x, int y, int l, int a)
{
    if (l <= 0 || a <= 0) { s->cx0 = 0; s->cy0 = 0; s->cx1 = s->larg; s->cy1 = s->alt; return; }
    s->cx0 = x < 0 ? 0 : x;
    s->cy0 = y < 0 ? 0 : y;
    s->cx1 = x + l > s->larg ? s->larg : x + l;
    s->cy1 = y + a > s->alt ? s->alt : y + a;
}

void sup_retangulo(superficie_t *s, int x, int y, int l, int a, uint32_t cor)
{
    int x0 = x < s->cx0 ? s->cx0 : x, y0 = y < s->cy0 ? s->cy0 : y;
    int x1 = x + l > s->cx1 ? s->cx1 : x + l, y1 = y + a > s->cy1 ? s->cy1 : y + a;
    for (int j = y0; j < y1; j++) {
        uint32_t *p = s->px + j * s->larg;
        for (int i = x0; i < x1; i++) p[i] = cor;
    }
}

/* Escurece o que já está pintado (sombra das janelas): cada canal * 5/8. */
void sup_escurecer(superficie_t *s, int x, int y, int l, int a)
{
    int x0 = x < s->cx0 ? s->cx0 : x, y0 = y < s->cy0 ? s->cy0 : y;
    int x1 = x + l > s->cx1 ? s->cx1 : x + l, y1 = y + a > s->cy1 ? s->cy1 : y + a;
    for (int j = y0; j < y1; j++) {
        uint32_t *p = s->px + j * s->larg;
        for (int i = x0; i < x1; i++) p[i] = ((p[i] >> 1) & 0x7F7F7F) + ((p[i] >> 3) & 0x1F1F1F);
    }
}

void sup_moldura(superficie_t *s, int x, int y, int l, int a, uint32_t cor)
{
    sup_retangulo(s, x, y, l, 1, cor);
    sup_retangulo(s, x, y + a - 1, l, 1, cor);
    sup_retangulo(s, x, y, 1, a, cor);
    sup_retangulo(s, x + l - 1, y, 1, a, cor);
}

static inline void ponto(superficie_t *s, int x, int y, uint32_t cor)
{
    if (x >= s->cx0 && x < s->cx1 && y >= s->cy0 && y < s->cy1)
        s->px[y * s->larg + x] = cor;
}

int texto_largura(const char *t)
{
    int n = 0;
    while (t[n]) n++;
    return n * 8;
}

void sup_texto(superficie_t *s, int x, int y, const char *t, uint32_t cor)
{
    for (; *t; t++, x += 8) {
        if (x >= s->cx1) break;
        if (x + 8 <= s->cx0) continue;
        const unsigned char *g = fonte_sagui[(uint8_t)*t];
        for (int j = 0; j < 16; j++)
            for (int i = 0; i < 8; i++)
                if (g[j] & (0x80 >> i)) ponto(s, x + i, y + j, cor);
    }
}

void sup_desenho(superficie_t *s, int x, int y, const desenho_t *d, int escala)
{
    for (int j = 0; j < d->altura; j++)
        for (int i = 0; i < d->largura; i++) {
            uint32_t cor;
            if (arte_cor(d->linhas[j][i], &cor))
                sup_retangulo(s, x + i * escala, y + j * escala, escala, escala, cor);
        }
}

void sup_copiar(superficie_t *s, int x, int y, const uint32_t *origem, int larg, int alt, int passo)
{
    int x0 = x < s->cx0 ? s->cx0 : x, y0 = y < s->cy0 ? s->cy0 : y;
    int x1 = x + larg > s->cx1 ? s->cx1 : x + larg, y1 = y + alt > s->cy1 ? s->cy1 : y + alt;
    for (int j = y0; j < y1; j++) {
        const uint32_t *o = origem + (j - y) * passo + (x0 - x);
        uint32_t *d = s->px + j * s->larg + x0;
        for (int i = 0; i < x1 - x0; i++) d[i] = o[i];
    }
}
