----BALAN GIORGIANA-LAVINIA----- 
-------------321CB--------------
-----------TEMA 2 PC------------


    Tema cuprinde 2 fisiere:

1) server.cpp

    Initial, am declarat macro-uri pentru coduri de eroare si structurile in
care retin datele primite sau de trimis catre clienti.

    Structura UDPMessage contine campurile enuntate in cerinta temei (topic, 
data_type si message).
    
    Structura TCP_client contine campurile necesare pentru a stoca toate
detaliile despre un client TCP (ID-ul sau, IP-ul, PORT-ul, socket-ul si
vectori in care retin topicurile cu SF0 / SF1 si mesajele stocate pentru
topicurile cu SF1 trimise pe perioada OFFLINE a clientului).

    In main verific daca este primit ca argument portul, fac configurarile
adreselor ca in laborator si deschid socketii TCP si UDP. Dupa deschiderea
acestora, ii adaug in multimea de file descriptori impreuna cu cel pentru
STDIN si recalculez cel mai mare file descriptor activ.

    Inainte de a asculta socketii initializez doi vectori pentru clienti
online si offline.

    In bucla while verific care socket / file descriptor este activ:

- UDPsocket: preiau mesajul venit de pe acest socket in structura creata
la inceput. Parsez continutul acesteia conform cerintei din tema si il stochez 
sub forma de string intr-o variabila.
    Dupa aceea, verific care dintre clientii online sunt inregistrati pentru topicul
curent si le trimit acest string. Apoi, verific pentru fiecare client offline
daca este inregistrat pentru acest topic, dar cu SF activat pe 1, aloc un
buffer si copiez continutul string-ului in acesta. Acest buffer nou creat
il adug in lista de not_sent a clientului offline, urmand ca la
reconectare sa fie trimis.

- TCPsocket: preiau informatiile de la clientul care doreste sa se conecteze si
verific daca acesta are un ID asemanator cu unul din cei deja ONLINE. Daca
se potriveste cu unul din acesti clienti, ii trimit mesajul de exit
(celui care doreste sa se conecteze, nu cel vechi) si il ignor, deoarece nu 
pot exista doi clienti cu acelasi ID in aceeasi sesiune.
    Daca acest client se reconecteaza, ii caut structura in vectorul de clienti
offline, ii actualizez datele (noul IP, PORT, socket) si il elimin din 
lista offline, reintroducandu-l in lista online. Dupa ce il reintroduc, 
ii trimit toate mesajele din lista de asteptare cu topicurile (cu SF=1)
care au fost trimise pe perioada offline a acestuia.
    Daca acest client se conecteaza pentru prima data ii aloc o structura
de tipul TCP_client si completez campurile cu datele acestuia. Daca
alocarea structurii nu reuseste (nu mai exista memorie sau diverse motive)
trimit clientului un mesaj de exit, acesta fiind fortat sa isi inchida
executia.

- STDIN_FILENO: Daca se citeste de la tastatura altceva in afara de comanda
"exit", aceasta se va ignora.
    Daca a fost citita comanda exit, atunci trimit mesajul "exit" la
toti clientii ONLINE urmand sa dezaloc memoria alocata pentru fiecare
client in parte si sa inchid socketii. La fiecare deconectare a unui
client afisez mesajul specificat in enuntul temei.

- Restul cazurilor sunt pentru situatia in care un client trimite date 
catre server.
    Aceste date sunt doar de forma unor string-uri si reprezinta comenzi
valide precum "subscribe topic SF" / "unsubscribe topic" / "exit".
    Daca un client a trimis comanda "exit", acesta va fi sters din lista
clientilor ONLINE si va fi introdus in lista clientilor OFFLINE afisand
mesajul corespunzator de deconectare.
    Daca se trimite comanda "subscribe topic SF", aceasta va fi parsata,
se va sterge vechea subscriptie pe acest topic din ambii vectori cu SF=0
si SF=1 pentru a evita duplicarea mai multor celule cu acelasi topic.
In functie de SF, topicul va fi introdus in vectorul corespunzator.
    Daca se trimite comanda "unsubscribe topic", topic-ul va fi sters
din una din cele doua liste de topicuri.



2) subscriber.cpp

    In subscriber, la fel ca in server, am folosit macro-uri pentru
coduri de eroare.

    In main verific numarul de argumente si salvez ID-ul, IP-ul si PORT-ul
subscriber-ului, deschid socket-ul prin care comunic cu serverul si
folosesc comanda setsockopt pentru a dezactiva algoritmul lui Neagle.
    Dupa configurarile adresei serverului si conectarea la acesta,
trimit ID-ul catre server.
    Daca vreo comanda din cele enumerate mai sus esueaza (result < 0)
executia acestui subscriber se va inchide.
    
    In bucla while verific daca este activ socket-ul cu serverul sau
file descriptor-ul STDIN.

- STDIN_FILENO: umplu cu zero-uri bufferul pentru a evita diferite probleme
si citesc de la tastatura comenzi.
    Daca se citeste exit, anunt serverul si inchid socket-ul.
    Daca se citeste subscribe / unsubscribe fac parsarile pentru
cele doua cazuri si trimit serverului string-ul care contine comanda completa.
   
    Orice alta combinatie invalida de comenzi sau cuvinte vor fi evitate.

- Socketul serverului: umplu cu zero-uri bufferul (acelasi motiv ca mai sus),
stochez datele in buffer (daca s-a receptionat rezultatul lui recv < 0 inchid
socket-ul si executia programului) si verific daca am primit comanda "exit",
caz in care inchid socketul serverului si executia programului. Orice alt
string venit de la server va fi afisat la consola deoarece acesta va veni deja
formatat conform cerintei temei.


3) Makefile
    
    Makefile-ul este cel din laborator cu mici modificari pentru a putea
fi rulat pentru aceasta tema.




