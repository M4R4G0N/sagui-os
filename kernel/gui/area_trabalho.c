/* area_trabalho.c - o fundo, o mascote, os ícones e a barra superior (H20, H28).
 *
 * O fundo não muda: é desenhado uma vez numa superfície guardada (degradê
 * verde, amarelo e azul, o sagui no canto e os ícones), e a cada
 * redesenho só a parte necessária é copiada. A barra superior é desenhada
 * por cima, porque o relógio muda. */
#include "gui.h"
#include "arte.h"
#include "console.h"
#include "memoria.h"
#include "processo.h"
#include "relogio.h"
#include "texto.h"
#include "video.h"
#include "disco.h"
#include "sistema.h"
#include "rede.h"

static uint32_t *fundo;
static int larg, alt;
static int minuto_mostrado = -1;
static char rede_mostrada[24] = "?";

typedef struct {
    const char *nome;
    const char *programa;
    const desenho_t *desenho;
} icone_t;

static const icone_t icones[] = {
    { "Arquivos",    "/bin/arquivos",    &arte_icone_arquivos },
    { "Terminal",    "/bin/terminal",    &arte_icone_terminal },
    { "Editor",      "/bin/editor",      &arte_icone_editor },
    { "Calculadora", "/bin/calculadora", &arte_icone_calculadora },
    { "Banana",      "/bin/banana",      &arte_icone_banana },
    { "Sagui 3D",    "/bin/sagui3d",     &arte_icone_sagui3d },   /* bônus B4 */
    { "Sobre",       "/bin/sobre-o-sistema",   &arte_icone_sobre },
    { "Cipó",        "/bin/cipo",        &arte_icone_cipo },       /* navegador (D032) */
    { "Pendrive",    "/bin/arquivos",    &arte_icone_pendrive },   /* só aparece com um pendrive conectado (H34) */
};
#define QTD_ICONES (int)(sizeof icones / sizeof icones[0])
#define ICONE_PENDRIVE (QTD_ICONES - 1)
static bool pendrive_visivel;
static char nome_pendrive[24] = "Pendrive";

static bool icone_visivel(int i) { return i != ICONE_PENDRIVE || pendrive_visivel; }
#define ICONE_X 20
#define ICONE_Y0 (BARRA_SUPERIOR + 16)
#define ICONE_PASSO 84   /* 8 ícones por coluna cabem em 768 linhas */
#define ICONES_COLUNA 8
#define ICONE_COLUNA_PASSO 110
#define ICONE_L 96
#define ICONE_A 88

int area_qtd_icones(void) { return QTD_ICONES; }
int area_qtd_programas(void) { return ICONE_PENDRIVE; }
const char *area_icone_nome(int i) { return icones[i].nome; }
const desenho_t *area_icone_desenho(int i) { return icones[i].desenho; }

void area_icone_retangulo(int i, int *x, int *y, int *l, int *a)
{
    *x = ICONE_X + (i / ICONES_COLUNA) * ICONE_COLUNA_PASSO;
    *y = ICONE_Y0 + (i % ICONES_COLUNA) * ICONE_PASSO;
    *l = ICONE_L; *a = ICONE_A;
}

/* Interpolação de um canal em inteiros. */
static uint32_t mistura(uint32_t a, uint32_t b, int num, int den)
{
    uint32_t r = 0;
    for (int s = 0; s <= 16; s += 8) {
        int ca = (int)((a >> s) & 255), cb = (int)((b >> s) & 255);
        r |= (uint32_t)(ca + (cb - ca) * num / den) << s;
    }
    return r;
}

static void texto_com_sombra(superficie_t *s, int x, int y, const char *t, uint32_t cor)
{
    sup_texto(s, x + 1, y + 1, t, 0x101010);
    sup_texto(s, x, y, t, cor);
}

static void desenhar_fundo(void)
{
    superficie_t s;
    sup_iniciar(&s, fundo, larg, alt);
    /* Degradê vertical nas cores do Brasil: verde em cima, amarelo no meio,
     * azul embaixo, em tons mais suaves que os da bandeira para o texto e os
     * ícones continuarem legíveis. */
    const uint32_t verde = 0x0B7A3E, amarelo = 0xE0B92A, azul = 0x14337E;
    for (int y = 0; y < alt; y++) {
        uint32_t c = y < alt / 2 ? mistura(verde, amarelo, y, alt / 2)
                                 : mistura(amarelo, azul, y - alt / 2, alt - alt / 2);
        uint32_t *p = fundo + y * larg;
        for (int x = 0; x < larg; x++) p[x] = c;
    }
    /* O mascote, grande, no canto de baixo à direita. */
    int esc = larg >= 1000 ? 5 : 3;
    int mx = larg - 32 * esc - 30, my = alt - 32 * esc - 40;
    sup_desenho(&s, mx, my, &arte_sagui, esc);
    texto_com_sombra(&s, mx + 16 * esc - texto_largura("SaguiOS") / 2, my + 32 * esc + 6, "SaguiOS", 0xFFFFFF);
    /* Ícones na coluna da esquerda, com o nome embaixo. */
    for (int i = 0; i < QTD_ICONES; i++) {
        if (!icone_visivel(i)) continue;
        int x, y, l, a;
        area_icone_retangulo(i, &x, &y, &l, &a);
        sup_desenho(&s, x + (l - 64) / 2, y, icones[i].desenho, 2);
        const char *nome = i == ICONE_PENDRIVE ? nome_pendrive : icones[i].nome;
        texto_com_sombra(&s, x + (l - texto_largura(nome)) / 2, y + 68, nome, 0xFFFFFF);
    }
}

bool area_iniciar(void)
{
    larg = (int)video_largura();
    alt = (int)video_altura();
    fundo = kmalloc((uint32_t)(larg * alt) * 4);
    if (!fundo) return false;
    desenhar_fundo();
    return true;
}

void area_fundo(superficie_t *s)
{
    sup_copiar(s, 0, 0, fundo, larg, alt, larg);
}

bool area_relogio_mudou(void)
{
    data_hora_t d;
    cmos_ler_data(&d);
    char r[24];
    rede_resumo(r, sizeof r);
    return d.minuto != minuto_mostrado || txt_compara(r, rede_mostrada) != 0;
}

void area_barra(superficie_t *s)
{
    static const char *semana[] = { "dom", "seg", "ter", "qua", "qui", "sex", "sáb" };
    sup_retangulo(s, 0, 0, larg, BARRA_SUPERIOR, 0x18202A);
    sup_retangulo(s, 0, BARRA_SUPERIOR - 1, larg, 1, 0x0B7A3E);
    /* Cabecinha do sagui (as linhas 2 a 17 do ícone "sobre") e o nome. */
    desenho_t cabeca = { arte_icone_sobre.linhas + 2, 32, 16 };
    sup_desenho(s, 6, 4, &cabeca, 1);
    sup_texto(s, 44, 4, "SaguiOS", 0xFFDF4A);
    sup_texto(s, 112, 4, "um sistema operacional escrito do zero por IA", 0x9AA8B8);
    data_hora_t d;
    cmos_ler_data(&d);
    minuto_mostrado = d.minuto;
    char hora[48];
    snformatar(hora, sizeof hora, "%s %02u/%02u/%u  %02u:%02u", semana[dia_da_semana(d.ano, d.mes, d.dia)],
               d.dia, d.mes, d.ano, d.hora, d.minuto);
    int xh = larg - texto_largura(hora) - 10;
    sup_texto(s, xh, 4, hora, 0xFFFFFF);
    /* Rede (D031): três barrinhas, verdes com IP, cinzas sem; e o estado. */
    rede_resumo(rede_mostrada, sizeof rede_mostrada);
    if (rede_mostrada[0]) {
        bool ok = rede_mostrada[0] >= '0' && rede_mostrada[0] <= '9';
        int xt = xh - 24 - texto_largura(rede_mostrada);
        sup_texto(s, xt, 4, rede_mostrada, ok ? 0xB8E8C8 : 0x9AA8B8);
        for (int b = 0; b < 3; b++)
            sup_retangulo(s, xt - 22 + b * 6, 16 - 4 * b, 4, 4 + 4 * b, ok ? 0x3CC870 : 0x5A6470);
    }
    klog("[GUI] relogio: %s rede: %s\n", hora, rede_mostrada[0] ? rede_mostrada : "-");
}

int area_icone_em(int x, int y)
{
    for (int i = 0; i < QTD_ICONES; i++) {
        if (!icone_visivel(i)) continue;
        int ix, iy, l, a;
        area_icone_retangulo(i, &ix, &iy, &l, &a);
        if (x >= ix && x < ix + l && y >= iy && y < iy + a) return i;
    }
    return -1;
}

/* Mostra ou esconde o ícone do pendrive (H34). Devolve true se mudou. */
bool area_pendrive(bool presente, const char *rotulo)
{
    char nome[24];
    txt_copia_n(nome, rotulo && rotulo[0] ? rotulo : "Pendrive", sizeof nome);
    if (presente == pendrive_visivel && txt_compara(nome, nome_pendrive) == 0) return false;
    pendrive_visivel = presente;
    txt_copia_n(nome_pendrive, nome, sizeof nome_pendrive);
    desenhar_fundo();
    return true;
}

static disco_t *primeiro_disco(void)
{
    for (int i = 0; i < MAX_DISCOS; i++) {
        disco_t *d = disco_indice(i);
        if (d && d->presente && !d->de_boot) return d;
    }
    return NULL;
}

/* Botão direito num ícone: no pendrive, "remover com segurança". */
void area_icone_direito(int i)
{
    if (i != ICONE_PENDRIVE) return;
    disco_t *d = disco_montado();
    if (d) disco_desmontar(d);
    else gui_aviso("O pendrive não está montado: pode remover.");
}

/* Abre o programa do ícone (H28). Ele não tem pai: ninguém espera por ele. */
void area_icone_abrir(int i)
{
    if (i < 0 || i >= QTD_ICONES) return;
    if (i == ICONE_PENDRIVE) {
        disco_t *d = primeiro_disco();
        if (!d) return;
        int r = disco_montar(d);
        if (r == ERRO_FORMATO) { gui_aviso("Este pendrive não está em FAT32: formato não suportado."); return; }
        if (r < 0 && r != ERRO_OCUPADO) { gui_aviso("Não consegui montar o pendrive."); return; }
        char *argv[] = { "arquivos", "/usb", NULL };
        int pid = processo_executar("/bin/arquivos", 2, argv, NULL);
        klog("[GUI] icone Pendrive -> /bin/arquivos /usb (pid %d)\n", pid);
        return;
    }
    char *argv[] = { (char *)icones[i].programa + 5, NULL };
    int pid = processo_executar(icones[i].programa, 1, argv, NULL);
    klog("[GUI] icone %s -> %s (pid %d)\n", icones[i].nome, icones[i].programa, pid);
}
