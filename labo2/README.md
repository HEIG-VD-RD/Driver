# Rapport de laboratoire 2

### Auteur : Rafael Dousse

## Compilation

Pour compiler et tester les programmes il suffit de lancer la commande `make` dans le répertoire du projet. Cela va compiler les différents programmes et les copier les exécutables dans le répertoire `export/drv` qui est le répertoire partager avec ma DE1-SoC. Il faut changer cette variable dans le Makefile si vous voulez changer le répertoire de destination.

## Exercice 2

Ici, on utilise une taille de 4096 bytes (4 KB) car c'est la taille standard d'une page mémoire. On va donc utiliser une taille qui permet de nous aligner facilement sur les limites de page. On utilise 0xFF200000 comme adresse car c'est une adresse spécifique qui correspond à une région mémoire physique réservée pour des périphériques particuliers sur notre système. Cela permet de garantir que l'espace de mémoire mappé correspond exactement à la région de mémoire que le périphérique utilise, permettant un accès direct et sécurisé aux registres du périphérique sans que l'on puisse interférer avec d'autres parties du système.

Si on utilise mmap() dans un context normal (sans UIO), on peut accéder à des adresses mémoires qui ne sont pas autorisées, ce qui peut causer des crashs du système. En utilisant mmap() avec UIO, on peut garantir que l'accès à la mémoire est sécurisé et limité à la région spécifique du périphérique, réduisant ainsi les risques d'erreurs et de crashs.          

 Les erreurs dans les drivers user-space ne provoquent pas de crash du noyau, rendant le système plus stable et le développement et le débogage sont souvent plus simples en espace utilisateur qu'en espace noyau. Par contre, l'accès aux périphériques peut être moins rapide comparé aux drivers en espace noyau à cause des changements de contexte entre espace utilisateur et noyau.  

## Exercice 5

Les trois méthodes pour attendre une interruption et avec l'utilisation de read(), de poll() et de select(). Ce sont 3 fonctions qui permettent de bloquer le programme jusqu'à ce qu'une interruption soit reçue. 
- read() : L'utilisation de read() permet de bloquer le programme jusqu'à ce qu'une interruption soit reçue. Elle est simple à utiliser mais ne permet pas de gérer plusieurs interruptions simultanées et bloque le programme jusqu'à ce que l'interruption voulue soit reçue. Cela peut être un problème si on veut gérer plusieurs interruptions simultanées ou par exemple si l'on souhaite utiliser SIGINT pour arrêter le programme car ctr-c va être détectée mais on risque de pas sortir dûne boucle while qui attend une autre interruption avec le read().
- poll(): poll() va faire la même chose que read() cet à dire attendre une interruption mais elle permet de gérer plusieurs interruptions simultanées. Elle a aussi plus de flexibilité sur son utilisation car elle permet de choisir le type d'événement à attendre (POLLIN, POLLOUT, POLLERR, etc.). Elle permet aussi de définir un timeout pour définir le temps que le programme va bloquer avant de continuer. Elle permet aussi d'être débloquer si un signal de type SIGINT est reçu et de gérer ce genre de cas spécifique d'une manière différente. Le problème avec cette fonction et que son implémentation et son utilisation sont plus complexes que read() et le fait de gérer plusieurs fichiers peut augmenter la complexité du code.     
- select(): select() fonctionne un peu comme poll() dans le sens où elle permet de gérer plusieurs interruptions simultanées et de définir un timeout. Elle permet aussi de gérer les signaux de manière spécifique. Par contre elle a une limitation sur le nombre de fichiers qu'elle peut gérer (1024). Il est plus conseillé d'utiliser poll() que select() car elle est plus flexible et plus performante.