Nexa_v1 - LAN Peer-to-Peer Messenger (C / WinSock2)

Description

Nexa_v1 est une application de messagerie Peer-to-Peer (P2P)
fonctionnant en réseau local (LAN), développée en C sous Windows avec
l’API WinSock2.

Fonctionnalités principales : - Découverte automatique des utilisateurs
via UDP Broadcast - Communication directe via TCP Unicast - Système
d’accusé de réception (ACK) - Multi-threading Windows - UUID
persistant - Pseudo unique basé sur UUID - Sauvegarde locale des
messages

Fonctionnement

1)  Découverte des utilisateurs Port UDP utilisé : 5000

L’application envoie périodiquement un message : HELLO;;

Les utilisateurs détectés sont stockés en mémoire comme peers actifs.

2)  Envoi des messages Port TCP utilisé : 6000

Processus : - Connexion TCP vers le destinataire - Envoi du message -
Attente d’un ACK - Si ACK reçu : message confirmé - Sinon : renvoi
automatique

3)  Réception des messages

Un serveur TCP est lancé en permanence. Les messages reçus sont
enregistrés dans : messages.txt

Système d’identité

uuid.txt -> Identifiant unique généré une seule fois pseudo.txt ->
Pseudo sauvegardé localement messages.txt -> Historique des messages
envoyés et reçus

Format du pseudo : NomChoisi#AB12

Le suffixe est généré à partir d’un hash déterministe basé sur l’UUID.

Threads utilisés

-   udp_listener : écoute les broadcasts UDP
-   tcp_listener : reçoit les messages TCP
-   sender_thread : envoie les broadcasts et renvoie les messages non
    confirmés

Synchronisation via CRITICAL_SECTION pour éviter les conflits d’accès
mémoire.

Compilation (MinGW)

gcc main.c -o Nexa_v1.exe -lws2_32

Utilisation

1.  Compiler le programme

2.  Lancer l’application sur plusieurs machines du même réseau local

3.  Choisir un pseudo au premier lancement

4.  Utiliser le menu :

5.  Messages envoyés et reçus

6.  Utilisateurs en ligne

7.  Changer pseudo

8.  Envoyer un message

9.  Quitter

Ports utilisés

UDP : 5000 TCP : 6000

Concepts techniques utilisés

-   UDP Broadcast
-   TCP Client/Server
-   Multi-threading Windows
-   Synchronisation (Critical Sections)
-   File chaînée pour la gestion des messages
-   Gestion mémoire dynamique
-   Persistance locale des données

Objectif pédagogique

Ce projet permet de : - Comprendre la programmation réseau bas niveau en
C - Manipuler WinSock2 - Gérer la concurrence (multi-threading) -
Concevoir une architecture P2P simple - Construire un projet technique
pour portfolio.
