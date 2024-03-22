Tema 2 PCom - Baban Mihai-Emilian 324CD


Pentru a implementa m-am folosit de scheltul din laboratorul 7.

Am folosit doua structuri:
- struct chat_packet care retine informatii importante dintr-un pachet
 trimis catre subscriber sau server
- struct client in care am retinut id-ul, socket-ul si topicul clientului

SERVER.C

-parsez port-ul ca un numar
-deschid 2 socketi pentru clientul tcp si clientul udp
-asociez adresa serverului folosind bind

void run_chat_multi_server(int tcp_sock, int udp_sock):
-folosesc funcția poll pentru a asculta
 și procesa evenimentele pe socketurile asociate clienților.
-aloc memorie pentru clienții conectați 
-aplic functia listen pe socket-ul tcp asteptand conexiuni de la clientii tcp
-atunci când un client se conectează, serverul verifică dacă acesta
 este un client nou sau unul existent deja.
-folosesc funcția recv_all() pentru a primi pachetele pe care clienții le trimit.
-atunci când un client se deconectează, serverul întrerupe 
 conexiunea și scoate socketul de client din multimea de citire.

SUBSCRIBER.C

-deschidem socket-ul tcp
-intr-o buclă infinită, clientul așteaptă evenimente 
 de la tastatură și de la server, folosind funcția poll().
-dacă se introduce o comandă la tastatură, aceasta este
 prelucrată și apoi trimisă la server 
-comenzile posibile sunt subscribe, unsubscribe și exit.