# Joc-algorismia

He creat aquest github per pujar i baixar els objectes de codi del joc d'algorísmia.
Per afegir objectes d'altres jugadors sense que us passin el codi, heu de descarregar el seu objecte (AIexemple.o), afegir-lo a la carpeta Game, i dins del makefile afegir " AIexemple.o" a la línia "EXTRA_OBJ = "  Per exemple:
EXTRA_OBJ = AIexemple.o AIMonchi AIMonchi4 AIAsesino
Ara make all i podeu jugar amb ells.

En el cas dels meus codis:
-- Recomano comprovar els vostres programes contra els més bàsics que he creat jo. Comenceu intentant guanyar-los.
- AIMonchi és bàsic i no l'importen altres jugadors
- AIAsesino és bàsic i no l'importen les ciutats (molt agressiu).


- AIMonchi3 és millor que AIMonchi i sap esquivar.

- AIMonchi4 té una estratègia concreta i una mica estranya. És millor que AIMonchi3.
- AIMonchi5 té estratègia semblant a AIMonchi3 i guanya ~50% de les partides contra AIMonchi4.

Reventem els d'EDA!
