# Labo 6
### Auteurs: Rafael Dousse
## Utilisation de l'application
Pour compiler le projet il suffit de faire `make` dans le dossier du projet. Une variable du makefile permet de choisir le dossier où se trouve la toolchain. De base, elle va chercher la toolchain dans le chemin de la VM reds. Le makefile va copier et coller les fichiers dans le dossier `/export/drv/` qui est partagé avec les DE1.
Donc chaque makefile va builder et copier les fichiers nécessaires.

## Exercice Chronomètre

Pour tester le chronomètre et après avoir insérer le module dans le kernel, il y a un fichier nommé `test_ex_chrono` qui intéragit avec le module et qui, quand exécuté, imprime la liste des tours enregistrés.

Il y a aussi des fichiers sysfiles qui se trouve dans le dossier `/sys/devices/platform/ff200000.drv2024/chrono`. Ces fichiers permettent de savoir si le chrono est actif, le temps actuel du chrono, la liste des tours enregistrés, le nombre de tours enregistrés ainsi que de savoir le mode de l'affichage des tours. Il est possible de changer ce mode en écrivant dans le fichier `laps_mode` avec les valeurs `0` ou `1` avec la commande `echo`.

### Utilisation des boutons

- Key0: Lance/arrete le chrono
- Key1: Enregistre un tour
- Key2: Affiche les tours enregistrés
- Key3: Reset le chrono 

## Problème du code

Je tiens a faire des remarques par rapport au code. Il y a des choses qui sont faite qui pourraient être amélioré ou faite différemment. La gestion des soustractions et des divisions n'est pas la meilleure manière de faire. J'aurai pu par exemple enregistrer le temps au lieur de ma structure temps dans la liste, ce qui m'aurait simplifier les calculs tours à afficher. La fin du chronomètre est faite de manière un peu brutale ainsi que l'affichage du premier tour dans le mode 1 qui n'est pas très propre . Je fais certaines opérations sur les itérateurs de la liste qui ne seraient peut être pas nécessaire à certains endroits mais comme ça fonctionnait, je n'ai pas voulu risqué de tout casser... 
Finalement, il manque les deux derniers points qui étaient demandés dans la consigne et qui concerne l'affichage des tours lors de la remise à zéro. La cause étant la longueur de ce labo ainsi que les différents test à réviser et labos à rendre en cette fin de semestre qui étaient nombreux. Si j'avais eu plus de temps, j'aurais pu faire ces deux points manquant ainsi que corriger les problèmes cités ci-dessus.
