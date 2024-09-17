# Labo 5
### Auteurs: Rafael Dousse
## Utilisation de l'application
Pour compiler le projet il suffit de faire `make` dans le dossier du projet. Une variable du makefile permet de choisir le dossier ou se trouve la toolchain. De base elle va chercher la toolchain dans le chemin de la VM reds. Le makefile va copier et coller les fichiers dans le dossier `/export/drv/` qui est partagé avec les DE1.
Donc chaque makefile va builder et copier les fichiers nécessaires pour chaque exercice.

## Exercice 1

Pour tester l'exercice 1, il y a 2 fichiers user-space qui permettent d'écrire dans le device. Cest fichiers sont `show_number` et `show_number_random`. Le premier est à utiliser de cette manière `./show_number <number>` ou numbers sont plusieurs valeures à écrire dans la fifo. Le deuxième est à utiliser de cette manière `./show_number_random` et il va écrire des valeurs aléatoirement dans la fifo.

Ensuite, pour tester les sysfiles, il faut se déplacer dans le dossier `/sys/devices/platform/ff200000.drv2024/show_number` et lire les différents fichier avec `cat`. Pour changer le temps du timer il faut écrire `echo <number> > change_time` ou number est un nombre entre 0 et 15000 ms.

Ce n'était pas vraiment précisé si le 0 devait être dans la fifo ou pas. J'ai décidé de le mettre mais de ne pas l'affiché, ce qui peut faire une seconde d'affichage où il n'y a rien entre 2 valeurs si un 0 est dans la fifo.

## Réponse exercice 4
Dans l'exercice 3 on utilisait un mutex pour protéger notre variable. Le problème est que l'on ne peut pas utiliser de mutex dans un handler d'interruption car le mutex peut endormir le processus et on ne peut pas faire ça dans un handler d'interruption. Donc dans l'exercice 4, on peut changer les mutex par des spinlock et rajouter le handler d'interruption. Les spinlock offrent une protection sans endormissement, ce qui les rend adaptés pour une utilisation dans les contextes d'interruption. J'ai donc utilisé spin_lock_irqsave et spin_unlock_irqrestore pour désactiver les interruptions locales lors des modifications de value. Cette approche prévient les interruptions imbriquées et protège efficacement contre la conccurence et les problèmes que l'utilisation de mutex aurait pu causer.

