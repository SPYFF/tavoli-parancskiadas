/*<Távoli parancskiadás szerver program>
 Copyright (C) <2014>  <Fejes Ferenc>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 
 Ez a program szabad szoftver; terjeszthető illetve módosítható a 
 Free Software Foundation által kiadott GNU General Public License
 dokumentumában leírtak; akár a licenc 3-as, akár (tetszőleges) későbbi 
 változata szerint.

 Ez a program abban a reményben kerül közreadásra, hogy hasznos lesz, 
 de minden egyéb GARANCIA NÉLKÜL, az ELADHATÓSÁGRA vagy VALAMELY CÉLRA 
 VALÓ ALKALMAZHATÓSÁGRA való származtatott garanciát is beleértve. 
 További részleteket a GNU General Public License tartalmaz.

 A felhasználónak a programmal együtt meg kell kapnia a GNU General 
 Public License egy példányát; ha mégsem kapta meg, akkor
 tekintse meg a <http://www.gnu.org/licenses/> oldalon.*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>

#define PORT_NO 1337

//függvény a parancs közelítő helyességének értelmezéséhez
int Parancs_ellenoriz(char* Buff);
//eljárás a kapott parancs végrehajtására és eredmény előállítására
void Parancs_vegrehajt(char* Buff);

int main()
{
	/*************************************************************************/
	/*Kiépítjük a kapcsolatot és feltöltjük az ehhez szükséges struktúrákat a*/
	/*                         szerverünk számára.                           */
	/*************************************************************************/

	int Sock_fd, Cli_fd; //socket nekünk a hallgatáshoz és a kliensnek
	struct sockaddr_in My_addr; //struktúra a cím információknak
	struct sockaddr_in Cli_addr; //struktúra a kliens információinak

	//létrehozunk egy socketet amin hallgatunk majd és figyeljük sikerül-e,
	//ha nem akkor a socket függvény -1 értékkel tér vissza
	if((Sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}

	//beállítjuk hogy a socket tartsa életben a kapcsolatot hosszan
	//és hogy újra használhassunk majd a címhez a portot
	int True = 1; //ehez kell egy igaz integer érték
	if(setsockopt(Sock_fd, SOL_SOCKET, SO_KEEPALIVE, &True, sizeof(int)) == -1)
	{
		perror("setsockopt");
		exit(2);
	}
	if(setsockopt(Sock_fd, SOL_SOCKET, SO_REUSEADDR, &True, sizeof(int)) == -1)
	{
		perror("setsockopt");
		exit(3);
	}

	//feltöltjük a saját információnkkal a struktúrát
	My_addr.sin_family = AF_INET;
	My_addr.sin_port = htons(PORT_NO);
	My_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(My_addr.sin_zero), '\0', 8); //kompatibilitás sockaddr-al

	//van egy socket-ünk és egy struktúránk az információkkal, most pedig
	//ezt a socketet társítjuk egy porthoz
	if(bind(Sock_fd, (struct sockaddr*) &My_addr, sizeof(struct sockaddr)) == -1)
	{
		perror("bind");
		exit(4);
	}

	//van egy socketünk porthoz tásrítva, most már figyelhetjük hogy
	//van-e bejövő kommunikációs kérelem
	printf("Várakozás kliensre\n");
	if(listen(Sock_fd, 10) == -1)
	{
		perror("listen");
		exit(5);
	}

	//ha szeretnének ránk kapcsolódni, akkor akár el is fogadhatjuk, amivel
	//kapunk egy kliens socket azonosítót, ami a továbbiakban felhasználható
	//ha üzenetet akarunk küldneni a kliensnek
	socklen_t Cli_len = sizeof(struct sockaddr_in); //kell az acceptnek
	//socklen_t típus igazából egy unsigned integer típus, az accpet ilyen 
	//típust vár harmadik argumentumként mint a kliens struktúra mérete
	//int-el is megy csak warning-ot dob a gcc fordításkor
	if((Cli_fd = accept(Sock_fd, (struct sockaddr*)&Cli_addr, &Cli_len)) == -1)
	{
		perror("accpet");
		exit(6);
	}

	printf("Kliens csatlakozott.\n");


	/*************************************************************************/
	/*Meg is volnánk a kapcsolat kiépítésével. Ez eddig lényegében papírforma*/
	/*Mi van eddig? Nos van egy socketünk amire (elvileg) már csatlakozott   */
	/*egy kliens is. Innentől kezdve jöhet a program azon része ami az adott */
	/*feladatra, vagyis az applikációs rétegbeli funkcionalitára vonatkozik  */
	/*send() és recv() függvényekkel fogunk információt cserélni a klienssel */
	/*                              a továbbiakban.                          */
	/*************************************************************************/

	//a kliens által kiadható parancsok listája, ezt fogjuk elsőként elküldeni
	//a kliensnek
	char Parancsok[] = 
	"ifconfig\n\
	-tetszőleges-tetszóleges interfész beállításainak lekérdezése: ifconfig ethX | -a\n\
	-tetszőleges interfész IP paramétereinek beállítása, módosítása:\n\
		ifconfig ethX IP_cím netmask NM broadcast BC\n\
	-interfész MTU beállítása: ifconfig ethX mtu xxx\n\
route\n\
	-routing bejegyzés felvitele:\n\
		route add –net network netmask [gw gateway] dev ethX\n\
	-routing bejegyzés törlése: route del –net network [...]\n\
	-alapértelmezett útvonal/átjáró felvitele: route add default gw gateway\n\
	-routing tábla lekérdezése: route –n\n\
arp\n\
	-ARP tábla kiiratása: arp -a\n\
	-ARP bejegyzés felvitele: arp -s IP_cím MAC_cím\n\
	-ARP bejegyzés törlése: arp –d IP_cím\n\
Parancslista: (parancsok)\n\
Kilépés: (vége)\n";

	//elküldjük a parancslistát a kliensünknek
	if(send(Cli_fd, Parancsok, 8192, 0) == -1)
	{
		perror("send");
		exit(7);
	}

	//innentől kezdve már várjuk a parancsot, kiadjuk és visszaküljük egy eredményt
	//mindezt végtelen ciklusban, amiből kilépést a kliens által küldött (vége)
	//parancs jelenti.
	char Buffer[8192]; //ide fogadjuk a parancsot és ebbe írjuk majd az eredményt
	while(1)
	{
		if(recv(Cli_fd, Buffer, 8192, 0) == -1)
		{
			perror("recv");
			exit(8);
		}

		//megnézzük hogy vége van-e és ha igen akkor kilépünk a cilusból
		//majd a programból is
		if(strcmp(Buffer, "(vége)") == 0)
		{
			break;
		}

		//ha a parancsokra kíváncsi a kliens akkor egyszerűen elküldjük neki ismét,
		//majd átiterálunk a ciklus további részén
		if(strcmp(Buffer, "(parancsok)") == 0)
		{
			if(send(Cli_fd, Parancsok, 8192, 0) == -1)
			{
				perror("send");
				exit(9);
			}

			continue;
		}

		//megvizsgáljuk érvényes-e a parancs amit kaptunk a klienstől
		if(Parancs_ellenoriz(Buffer))
		{
			//ha érvényes (vagy legalábbis átment a vizsgálaton) akkor végrehajtjuk
			Parancs_vegrehajt(Buffer);
		}
		else //ha nem érvényes akkor hibát küldönk vissza
		{
			char Hiba[] = "Hibás parancs!\n";
			if(send(Cli_fd, Hiba, 8192, 0) == -1)
			{
				perror("send");
				exit(10);
			}

			//fontos, nehogy két send legyen egymás után
			continue;
		}

		//Ha érvényes volt a parancs akkor már a Buffer-ben az eredmény
		//amit nyugodt szívvel küldhetünk vissza a kliensnek
		if(send(Cli_fd, Buffer, 8192, 0) == -1)
		{
			{
				perror("send");
				exit(11);
			}
		}
	}

	close(Cli_fd); //zárjuk a kliens socketet
	close(Sock_fd);	//zárjuk a szerver socketet
	
	return 0;
}

/*****************************************************************************/
/*                            Függvény definíciók                            */
/*****************************************************************************/

//kisebb ellenőrzés a kapott parancson
int Parancs_ellenoriz(char* Buff)
{
	//lehet valaki trükközik ; jellel tagolt parancsokkal
	if(strchr(Buff, ';') != NULL)
	{
		return 0;
	}


	if(strstr(Buff, "ifconfig"))
	{
		return 1;
	}

	if(strstr(Buff, "arp"))
	{
		return 1;
	}

	if(strstr(Buff, "route"))
	{
		return 1;
	}

	//ezeken kívül mást nem szabad hogy tartalmazzon
	return 0;
}

//kis érdekesség: a parancsot végrehajtjuk és a kimenetével nyitunk egy
//pipe-ot, és ebből a pipe-ból egy hagyományos szöveges fájlhoz hasonló
//módon kiolvassok a kimenetet és felülírjuk vele a buffer tartalmát
void Parancs_vegrehajt(char* Buff)
{
	//infó: man popen
	//végrehajtja a Buff-ban lévő parancsot és a kimenettel egy pipe-ot
	//hoz létre amiből kilvashatjuk azt
	FILE* Output = popen(Buff, "r");
	if(Output == NULL)
	{
		perror("popen");
		exit(12);
	}

	//soronként olvasunk a kimenetből, ehhez foglalunk kis helyet
	char* Beolvaso = (char*) malloc(1024 * sizeof(char));
	Buff[0] = '\0'; //üres sztringet csinálunk a Buff-ból
	while(fgets(Beolvaso, 1024, Output) != NULL)
	{
		//minden sort hozzáfűzűnk a már üres Buff-hoz
		strcat(Buff, Beolvaso);
	}

	//nincs további szükségünk a Beolvaso sztringre, felszabadítjuk
	free(Beolvaso);
	pclose(Output);
}