# Joc-algorismia

He creat aquest github per pujar i baixar els objectes de codi del joc d'algorísmia.
Per afegir objectes d'altres jugadors sense que us passin el codi, heu de descarregar el seu objecte (AIexemple.o), afegir-lo a la carpeta Game, i dins del makefile afegir " AIexemple.o" a la línia "EXTRA_OBJ = "  Per exemple:
EXTRA_OBJ = AIexemple.o AIMonchi.o AIMonchi4.o AIAsesino.o
Ara make all i podeu jugar amb ells. Afegiu qualsevol programa que pugui guanyar contra els dummies, i comparem!

Reventem els d'EDA!

# Objectes Martón 

Recomano comprovar els vostres programes contra els més bàsics que he creat jo. Comenceu intentant guanyar-los.

- AIMonchi és bàsic i no l'importen altres jugadors
- AIAsesino és bàsic i no l'importen les ciutats (molt agressiu).
- AIMonchi3 és millor que AIMonchi i sap esquivar.
- AIMonchi4 persegueix i mata dins ciutats, seguint una estratègia concreta. És millor que AIMonchi3.
- AIMonchi5 és una millora d'AIMonchi3 però també intenta matar com AIAsesino. Guanya ~50% de les partides contra AIMonchi4.

# Objectes Ramiro
Són per a Mac. 
- AIAsesino3 és bàsic. Sap esquivar, ataca, no utilitza costos. 






