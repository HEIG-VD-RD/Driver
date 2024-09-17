# Labo 4 - Développement de drivers kernel-space I
## Auteur : Rafael Dousse

# Compilation

Pour compiler le module, il suffit de se placer dans le répertoire du module et de lancer la commande `make`. Les fichiers .ko et les fichiers test s'il y en a seront copié dans le répertoire /export/drv.
J'ai rajouté une variable `USE_VM` dans le Makefile qui me permet juste de choisir les chemins pour le Kernel et la toolchain. Si elle est à 1, alors le chemin est celui de la VM, sinon c'est celui de mon ordinateur.