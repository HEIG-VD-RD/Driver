# Labo 1

## Auteur: Rafael Dousse

## Exercice 1


Dans l'exemple donné qui se trouve dans les instructions de l'exercice, on peut voir différentes commandes utilisé qui nous montre comment accéder aux valeurs des registres de notre carte. 

```
SOCFPGA_CYCLONE5 # md.b 0x80008000 0x1
80008000: 46    F

SOCFPGA_CYCLONE5 # md.w 0x80008000 0x1
80008000: 4c46    FL

SOCFPGA_CYCLONE5 # md.l 0x80008000 0x1
80008000: eb004c46    FL..

SOCFPGA_CYCLONE5 # md.b 0x80008000 0x4
80008000: 46 4c 00 eb    FL..

SOCFPGA_CYCLONE5 # md.w 0x80008000 0x4
80008000: 4c46 eb00 9000 e10f    FL......

SOCFPGA_CYCLONE5 # md.l 0x80008000 0x4
80008000: eb004c46 e10f9000 e229901a e319001f    FL........).....
```
Dans la première ligne, on va accèder à 1 byte d'un mot de 8 bit, donc on affiche ce qui se trouve à l'adressee donné et on voit qu'on a un 46 en hexadécimal et qui représente un F en ASCII (0x46 = 70 en décimal et qui est égal à F dans la table ASCII). Après on accède à un mot de 16 bits et obtient donc 4c64. On peut remarquer que l'on a un système en little endian. Puis on accède à un mot de 32 bits.
Ensuite, on accède à 4 mots de 8 bits c'est pour ça qu'on à un affichage différent. Pareil pour des mots de 16 et 32 bits. La dernière ligne est un accès de 4 mots de 32 bits donc le premier mot de 64 bits (qu0n peut voir aussi dans le troisième appel) et les 3 mots qui suivent dans l'adresse.
Si on écrit sur une adresse qui n'est pas aligné, on risque soit d'avoir une erreur soit de lire quelque chose d'érroné ou que la carte redémarre. Dans mon cas j'ai eu différents résultat, soit j'ai réussi à lire ce qu'il y avait a l'adresse donné mais on peut douter des valeurs donné, soit ça m'a reboot la carte en me donnant un message d'avertissement qui me conseil de lire un README sur un accès non aligné:
```
SOCFPGA_CYCLONE5 # md.b 0x01010101 0x9
01010101: ea 02 21 41 ea c3 31 21 62    ..!A..1!b

SOCFPGA_CYCLONE5 #  md.l 0x01010101 0x1
01010101:data abort

    MAYBE you should read doc/README.arm-unaligned-accesses
```

Pour écrire la valeur binaire des switches et les écrire sur les LED il a fallu que j'aille regardé la documentation qu'on nous a donné afin de savoir quel registre/adresses utilisé. La lecture des boutons se fait sur les sliders swiches a l adresse 0xFF00040 et l'écriture de la valeur lu se fait sur les RED LEDS a l adresse 0xFF200000.
Les commandes utilisées sont:
- Pour lire 
```
SOCFPGA_CYCLONE5 # md.w 0cFF200040 0x1
ff200040: 03ff    ..
```
- Pour écrire:
`SOCFPGA_CYCLONE5 # mw.w 0xFF200000 0x03ff`

J'ai utilisé md.w et mw.w car comme les leds sont sur 10 bits, 'w' nous permet de lire et d'ecrire sur 16 bits.

## Exercice 2

Pour définir des variables d'environnement, on peut se servir de la commande setenv. Dans le contrôle des scripts, la commande autoscr est utilisée pour exécuter un script stocké en mémoire. Les boucles `while` et les `if` sont utilisables pour gérer le comportement du script.

Les adresses des afficheurs 7 segments HEX3-HEX0 et HEX5-HEX4 sont respectivement 0xFF200020 et 0xFF200030. Voici un le script qui alterne l'affichage sur ces segments :
```
setenv autoscr "while true; 
                do mw.l 0xFF200020 0x36363636;
                mw.l 0xFF200030 0x3636; 
                sleep 1;  
                mw.l 0xFF200020 0x49494949;
                mw.l 0xFF200030 0x4949;
                sleep 1;
                done"
```

Après avoir executer la commande, il faut utiliser 
`run autoscr` pour lancer le scripte.

## Compiler les exercices

Pour compiler et pouvoir exécuter les fichiers sur la FPGA, j'ai écrit un `Makefile` qui s'occupe de lancer les compilations et la copie des exécutables dans le bon répertoire. Il faut donc que le `Makefile` soit dans le même répertoire où se trouve les fichiers `.c`. Ensuite, il faut juste s'assurer d'utiliser le bon compilateur en vérifiant le `Makefile` que la variable CC soit le compilateur souhaiter et il faut aussi vérifier que le chemin de la variable `EXPORTDIR` soit bien le chemin du répertoire partagé avec la carte.
Ensuite, il faut juste exécuter la commande 
```bash
make
``` 
pour compiler les fichiers et la commande 
```bash
make clean
```
pour supprimer les fichiers exécutables créer.

Finalement, il faut se connecter a la carte avec `picocom -b 115200 /dev/ttyUSB0` pour pouvoir se connecter à la carte et aller sur le répertoire partager pour exécuter les fichiers transférer avec la commande `./<nomFichier>`.

### Exercices 

Pour les exercices demandé, j'ai utilisé des bouts de codes qui viennent du documment `Using_Linux_on_the_DE1-SoC` qui donne des explications pour utiliser la carte ainsi que des exemples de codes. Cela m'a permit de mieux comprendre comment faire du code qui va lire les adresses voulues et nous permettre d'actualiser les valeurs qui y sont contenues. 
J'ai donc repris quelques define qui, dans leur code, sont enregistré dans un fichier `address_map_arm.h` qui me permet dedéfinir les adresses de base, la taille ou l'adresses des LEDS.
J'ai aussi reprit les fonctions qui permettent d'initialiser le mapping de la mémoire avec l'utilisation de `mmap()` (qui permet de mapper une adresse physique dans l'espace d'adressage virtuel du processus) et le "dé-mapping". 
J'ai aussi fait un handler qui permet l'interruption et la finalisation correct du programme en utilisant les touches ctrl+c car si ça n'est pas fait alors il est difficile de quitter le programme. On peut le faire avec ctrl+z mais ça ne va pas éteindre les LEDS ou même faire appel à la fonction `cleanupMemoryMapping` qui permet de libérer l'espace d'adressage  qu'on a utilisé lors du mappage. 
Une dernière fonction à été écrite qui prends des arguments variable en paramètre et qui permettent de mettre les leds à 0x0 (donc de les éteindre)

Ces fonctions sont utilisées dans les trois fichiers d'exercices. Une chose que j'aurai pu faire et de les mettre dans un fichier `util.h`. 
L'exercice 4 à deux versions de code car je n'était pas sure de ce qui était demandé dans la consigne. La première version bouge le texte vers la gauche/droite que quand on garde les boutons Key1/Key0 (respectivement) appuyé et sinon ça ne défile pas.
La deuxième versions de l'exercice va faire défiler le message vers la gauche/droite automatiquement et va changer de sense que lorsque l'on appuie sur l'un des deux boutons. Si le message arrive au bout alors il s'arrête. Le changement de sens n'est pas immédiat et il faut parfois laisser le boutons appuyer plus ou moins 1/2 secondes pour voir opérer le changement de sens. Cela est surement due au fait qu'on vérifie si l'un des deux bouton est appuyé dans la boucle et comme il y a plusieurs instructions qui se suivent alors ça prends un "petit" peu de temps jusqu'à la prochaine vérification. Pour remédier à cela on devrait gérer les boutons comme une interruption.