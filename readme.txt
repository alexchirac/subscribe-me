#Copyright Chirac Alexandru-Stefan 323CA 2022-2026


Pentru aceasta tema am folosit scheletul laboratorului 7.

Am ales sa fac implementarea in C++ deoarece faciliteaza redimensionarea 
vectorilor. Avand in vedere ca am avut de lucru cu un numar variabil de
topic-uri si clienti, acesta a fost un factor foarte important de luat in
considerare.

Implementare subscriber:

Imediat dupa abonare subscriber-ul trimite catre server un mesaj cu id-ul sau
pentru ca acesta sa-l poata procesa.

Subscriber-ul are doua functionalitati principale: primirea unui mesaj de la
server si primirea unui mesaj de la tastatura.

Daca s-a primit mesajul "exit" de la tastatura inchid programul si inchid
socket-ul conectat la server.

Daca s-a primit unul din mesajele "subscribe"/"unsubscribe" de la tastatura,
trimit mesajul primit spre server, pentru a sti cum sa proceseze topic-ul
ce urmeaza sa fie trimis, iar apoi trimit topic-ul.

Pentru orice alta comanda primita de la tastatura afisez mesajul "INVALID 
COMMAND!"

Daca primesc un mesaj de la server, il procesez in functie de tipul mesajului
si il afisez.

Implementare server:

In server m-am folosit de un vector de subscriberi pentru a retine id-ul lor si
topic-urile la care sunt abonati.

In cazul in care se primeste de la tastatura comanda "exit", opresc programul,
inchid toate socket-urile deschise si eliberez toata memoria ocupata de 
subscriberi.

In cazul primirii unei comenzi de "Subscribe" de la un client TCP, sparg topic
primit in token-uri, si adaug topic-ul spart la vectorul de topic-uri al
subscriber-ului respectiv.

In cazul primirii unei comenzi de "Unsubscribe" de la un client TCP, sterg
topic-ul dat din vector-ul subscriber-ului.

In cazul primirii unui mesaj udp, verific ce subscriberi sunt abonati la
topic-ul dat si le trimit acestora mesajul.

Pentru receptionarea si prelucrarea mesajului udp am folosit o structura care
stocheaza in primii 50 de octeti topic-ul, urmatorul octet este tipul 
mesajului, iar urmatorii 1500 de octeti sunt rezervati pentru mesajul in sine.

Pentru eficientizarea programului am folosit functiile "send_all" si "recv_all"
sa trimita si sa receptioneze fix octetii utili, fara reziduuri.