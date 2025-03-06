# Zadání jednotlivě
Rozdělení co kdo dělá, datum vytvoření 6.3. 2025 


## Rozdělení:
Jára: Logika sliceru - C++ a WASM 

MKZ: UI sliceru - všechno v THREE.JS 

Michal: Django - hlavně redakční systém

## Do 9.3. 2025

*Jára:* Dodělat generování G-code (obecně, ještě ne pro jednotlivé tiskárny)

- Naslicovat model (optimálně)
- vygenerovat kontury 
- výplň (zatím basic)
- zapsat instrukce do G-code

*MKZ:* Odstranit současné bugy, implementovat manipulaci s modelem: 

- Pozice (X, Y)
- Scale
- Rotace (podle Z)

Všechno přímo myší, bez nějakého panelu. (Může být třeba optional vysouvací)


*Michal:* V projektu je prázdná aplikace `resources` na které poběží blog. Přidat tyto funkce: 

- !! Tohle dělat v našem oficiálním repozitáři [2024_RP_3E_N3XT](https://github.com/Jaromir007/2024_RP_3E_N3XT)

- Nahrát článek v .md souboru
- .md se vyrendruje do HTML a uloží se jako nová stránka (url) v sekci blog (aby byl článek dohledatelný v browserech)
- Zatím klidně bez přihlašování - prostě udělat stránku upload, může tam být i text field
- nová stránka pak bude vypadat: `httpts://n3xt.cz/resources/<ten nový článek>` 