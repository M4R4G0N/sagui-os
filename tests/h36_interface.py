#!/usr/bin/env python3
"""H36 - barra de tarefas, minimizar, menu Iniciar e ícone de rede.

Critério: com duas janelas abertas, o botão de minimizar esconde uma (ela
continua na barra de tarefas), o botão dela na barra a traz de volta, e o
menu Iniciar abre um programa. Com a placa de rede, a barra de cima mostra
o IP do DHCP.
"""
import os
import re
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from qemu import Maquina, verificar, imagem_de_teste, cor_do_pixel  # noqa: E402

AZUL = (0x33, 0x66, 0xCC)
TAREFAS_Y = 768 - 17          # meio da barra de tarefas


def janela(serial, nome):
    m = re.search(rf'janela (\d+) criada por \d+: "{nome}" (\d+)x(\d+) em \((\d+),(\d+)\)', serial)
    return dict(id=int(m.group(1)), l=int(m.group(2)), a=int(m.group(3)), x=int(m.group(4)), y=int(m.group(5)))


def main():
    extras = ["-netdev", "user,id=n0", "-device", "e1000,netdev=n0"]
    with Maquina("h36", "ide", imagem=imagem_de_teste("/testes/duas-janelas", ["--vesa"]), extras=extras) as m:
        m.esperar_serial("janela Janela B: aberta", 120)
        time.sleep(0.5)
        s = m.serial()
        a, b = janela(s, "Janela A"), janela(s, "Janela B")
        # A começa embaixo de B; o botão dela na barra (o primeiro) a traz para a frente.
        m.clique(116 + 60, TAREFAS_Y)
        time.sleep(0.5)
        png = m.captura("H36-barra-de-tarefas")
        verificar(cor_do_pixel(png, a["x"] + 100, a["y"] + 100) == AZUL, "o botão da barra de tarefas trouxe A para a frente")

        # Minimizar: o botão à esquerda do X.
        mx = a["x"] + a["l"] + 4 - 2 - 40 + 8
        m.clique(mx, a["y"] + 12)
        m.esperar_serial(f"janela {a['id']} minimizada", 10)
        time.sleep(0.5)
        png = m.captura("H36-minimizada", guardar=False)
        verificar(cor_do_pixel(png, a["x"] + 10, a["y"] + 30) != AZUL, "A sumiu da tela ao minimizar")
        m.clique(116 + 60, TAREFAS_Y)
        m.esperar_serial(f"janela {a['id']} restaurada", 10)
        time.sleep(0.5)
        png = m.captura("H36-restaurada", guardar=False)
        verificar(cor_do_pixel(png, a["x"] + 10, a["y"] + 30) == AZUL, "o botão da barra restaurou A")
        m.clique(116 + 60, TAREFAS_Y)
        m.esperar_serial(f"janela {a['id']} minimizada", 10)
        verificar(True, "clicar no botão da janela que está na frente minimiza")

        # Menu Iniciar: o segundo item é o Terminal.
        m.clique(50, TAREFAS_Y)
        time.sleep(0.5)
        m.captura("H36-menu-iniciar")
        menu_y = 768 - 34 - (30 + 9 * 38 + 6)
        m.clique(100, menu_y + 30 + 1 * 38 + 19)
        m.esperar_serial("icone Terminal -> /bin/terminal", 10)
        verificar(True, "o menu Iniciar abriu o Terminal")

        m.esperar_serial("[REDE] DHCP: ip 10.0.2.15", 30)
        m.esperar_serial("rede: 10.0.2.15", 10)
        m.captura("H36-icone-rede")
        verificar(True, "a barra de cima mostra o IP do DHCP")
    return 0


if __name__ == "__main__":
    sys.exit(main())
