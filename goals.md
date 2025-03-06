# N3XT - SLICER

Děláme webový slicer, který poběží na všech zařízeních (windows, linux, android… počítač i mobil). 

Chceme, aby byl slicer funkční a plně použitelný na každodenní potřeby běžných uživatelů. → Necílíme na to, abychom měli všechny funkce a nastavení jako v reálných slicerech, ale zároveň chceme poskytnout možnost nastavit si klíčové funkce.  Těmi jsou: 

- Výběr materiálu
- Výběr tiskárny
- Výška vrstvy
- Rychlost tisku
- Upravit teplotu podložky a extruderu
- Výplň (%)
- Podpěry (ANO/NE)
- Přidat Brim

Uživatel si teké může nastavit pozici, rotaci a velikost modelu (všechno ve 2D, osa Z zůstane fixní). Slicer automaticky upozorní na chyby (př. nevejde se na podložku, je mimo atd..): 

- Pozice (X, Y)
- Scale
- Rotace (podle Z)

Všechno nastavení se potom uloží pro WASM modul, který všechno vezme v potaz a naslicuje model. 

Vygeneruje G-Code, který se potom zobrazí na tiskové podložce. Uživatel si ho bude moct stáhnout. 

## N3XT - WEB

Na webu bude i blog s články o projektu - vysvětlení jak slicování funguje, ukázky kódu atd.. 

Pro začátek bude formátování v Markdown. Články může  nahrávat jenom admin. Funkce: 

- Nahrát článek v .md souboru
- Zvolit kategorii, a tagy
- .md se vyrendruje do HTML a uloží se jako nová url v sekci blog (aby byl článek dohledatelný v browserech)
- Poté možnost upravovat články, mazat, měnit kategorii atd..

