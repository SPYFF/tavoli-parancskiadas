/*/*<Távoli parancskiadás kliens program>
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
#include <arpa/inet.h>

#define PORT_NO 1337

int main()
{
	/*************************************************************************/
	/*Kiépítjük a kapcsolatot és feltöltjük az ehhez szükséges struktúrákat a*/
	/*                          kliensünk számára.                           */
	/*************************************************************************/
	int Sock_fd; //socket nekünk a kapcsolódáshozhoz
	struct sockaddr_in Srv_addr; //struktúra a cím információknak

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

	//feltöltjük a szerver információival a struktúrát, amihez kapcsolódunk
	Srv_addr.sin_family = AF_INET;
	Srv_addr.sin_port = htons(PORT_NO);
	//loopback címen fogunk a szerverhez csatlakozni, az inet_addr pedig
	//a sztringből egy belső ábrázolású IP címet csinál
	Srv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(&(Srv_addr.sin_zero), '\0', 8); //kompatibilitás sockaddr-al

	//most pedig kapcsolódunk a szerverhez
	if(connect(Sock_fd, (struct sockaddr*)&Srv_addr, sizeof(struct sockaddr)) == -1)
	{
		perror("connect");
		exit(4);
	}

	/*************************************************************************/
	/*Eddig tart egy szerverhez való kapcsolódási kísérlet kódja, itt kell   */
	/*egy socket amivel kapcsolódunk, valamit egy struktúra amiben a szerver */
	/*információi és címe van amihez kapcsolódni szeretnénk. Innentől jöhet a*/
	/*    kliens applikációs rétegbeli funkcionalitásának implementálása     */
	/*************************************************************************/

	char Buff[8192]; //itt fogjuk majd tárolni a beérkező üzenetét a szervernek
	//továbbá a kiküldendő parancsunkat is ide olvassuk majd be

	//elsőként a kliens fogadj a szervertől a parancsok listáját
	if(recv(Sock_fd, Buff, 8192, 0) == -1)
	{
		perror("recv");
		exit(5);
	}

	//megjelenítjük a parancsok listáját
	printf("%s\n", Buff);


	while(1)
	{
		printf("********************************************************************\n");
		printf("\nGépelje be a kívánt parancsot:\n");
		fgets(Buff, 512, stdin); //bekérjük a parancsot
		Buff[strlen(Buff) - 1] = '\0'; //soremelést levágjuk


		//ha ki szeretnénk lépni
		if(strcmp(Buff, "(vége)") == 0)
		{
			if(send(Sock_fd, Buff, 8192, 0) == -1)
			{
				perror("send");
				exit(6);
			}

			break; //megszakítjuk a ciklust
		}

		//elküldjük a parancsot a szervernek
		if(send(Sock_fd, Buff, 8192, 0) == -1)
		{
			perror("send");
			exit(7);
		}

		//fogadjuk a szerver válaszát
		if(recv(Sock_fd, Buff, 8192, 0) == -1)
		{
			perror("recv");
			exit(8);
		}

		//és a lényeg: a kapott eredményt kiírjuk!
		printf("%s\n", Buff);

	}

	close(Sock_fd);
	return 0;
}