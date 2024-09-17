# Rapport du laboratoire 3 driver
### Auteur : Rafael Dousse

## Compilation des fichiers et chargement des modules
Pour compiler les fichiers il faut lancer la commande `make` dans le dossier `flifo_module`. La commande va appeler le fichier Makefile qui va compiler les fichiers pour le driver ainsi que le fichier test et les copier dans le dossier /export/drv qui est le dossier de partage avec la DE1.
Puis finalement executer `insmod flifo.ko` pour charger le module dans la De1.

## Exercice 1
Sur l'image qui suit on peut voir que le fichier testDRV a le même majeur et mineur que le fichier /dev/random. 

![alt text](image.png)

La lecture du fichier qui a les même majeur et mineur que le fichier `/dev/random` nous renvoie des valeurs aléatoires mais la on n'arrive pas à le lire car ça affiche des caractères spéciaux. 

ex:
```bash
    *������(�i\�j�VC�;�,�V�wo�h���h��&�v5S,�6�?Ĳ��Hfʗ��V�F��3�'������o��%��΄C�ƇPی�(v��`��]5���4� F����V�fgD�es��[�JF"-y�X�h�4�m<�T-���c�v.ۦ�@�
I��~�Q-p��ĩu#뽨'��x%1g��O֧8�|��d
                                BO��D�𹫽��Vp+��OnT��R�:e�,. �z���_��ںQR �9�w�����"ڵ��!�sv��J8�'�&���?vmYE������"���l��y��L���T����͈��"���W�X{�(���I�:���λ�h��r��~<�, �&uh���p���y��A8��'eъ�ֽ
```


## Exercice 2   
L'image montre la lecture du fichier `/proc/device` et on peut voir le périphérique `ttyUSB0` avec le majeur 188.
![alt text](image-2.png)


## Exercice 3

``` bash
rafou@rafou:/sys$ find -name ttyUSB0
find: ‘./kernel/tracing’: Permission denied
find: ‘./kernel/debug’: Permission denied
./class/tty/ttyUSB0
./devices/pci0000:00/0000:00:14.0/usb1/1-2/1-2:1.0/ttyUSB0
./devices/pci0000:00/0000:00:14.0/usb1/1-2/1-2:1.0/ttyUSB0/tty/ttyUSB0
find: ‘./fs/pstore’: Permission denied
find: ‘./fs/bpf’: Permission denied
./bus/usb-serial/devices/ttyUSB0
./bus/usb-serial/drivers/ftdi_sio/ttyUSB0
rafou@rafou:/sys$ lsmod | grep ftdi
ftdi_sio               69632  1
usbserial              69632  3 ftdi_sio
rafou@rafou:/sys$ 
```
