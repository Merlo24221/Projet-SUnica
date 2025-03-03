# Projet-SUnica

Le projet Sunica vise à développer un compteur de fréquentation avec transmission des données par satellite. Il repose sur deux grandes fonctions principales : le comptage des passages et l’envoi des données.

1. Le comptage des passages
Pour compter le nombre de passages et déterminer leur sens (entrée ou sortie), nous utilisons deux capteurs à ultrasons. Lorsque quelqu’un passe, l’ordre de détection par les capteurs permet de savoir s’il s’agit d’une entrée ou d’une sortie.

Afin d’optimiser la consommation d’énergie, nous avons ajouté deux capteurs PIR placés en amont des capteurs à ultrasons. Ces capteurs détectent la présence d’une personne et activent le système uniquement en cas de besoin. Lorsqu’aucun mouvement n’est détecté, le système se met en veille pour économiser l’énergie.

2. L’envoi et la réception des données
Les données recueillies sont transmises via le satellite Echostar. À la réception, elles sont traitées avec le logiciel Node-RED, qui permet de visualiser et de gérer les informations en temps réel.

Les données sont ensuite enregistrées dans un fichier Excel (format .xlsx) sous la forme suivante :
 Date , Heure , Entrées : 00 ,Sorties : 00


 Resumé du fonctionnement
 
lorsque le capteur PIR detecte une présence , il active le systeme , le dispositif de comptage se met ensuite en place pendant un temps fixé. L’entrée ou la sortie d’une personne n’est prise en compte que si elle passe successivement devant les deux capteurs à ultrasons, permettant ainsi de déterminer avec précision le sens du passage et d'eviter les erreurs de comptage. Une fois le temps ecoulé le système de comptage se met en pause pour laisser place à l'envoi et la reception des données grace satellite. 

Pour une explication plus détaillée, veuillez consulter le document intitulé "Last Presentation".
