#include <Comio.h>

/*
 * Allocation des variables globales
 */
// index
unsigned char comio_index_N[COMIO_MAX_N];
unsigned char comio_index_A[COMIO_MAX_A];

// memoire "partagée"
unsigned char comio_mem[COMIO_MAX_M];

callback_f comio_fonctions[COMIO_MAX_FX];

/*
 * Fonctions internes
 */
int _comio_lire_trame(unsigned char *op, unsigned char *var, unsigned char *type, unsigned int *val)
{
	int c;
	unsigned char etat=0;
	
	while(etat<5)
	{
		// 5 essais pour obtenir des données
		for(int i=0;i<5;i++)
		{
			c=Serial.read();
			if(c>=0)
				break;
			delay(5); // 5 milliseconde entre 2 lectures
		}
		
		if(c<0)
			return 0; // pas de données => sortie direct "sans rien dire"
		
		// automate à états pour traiter les différents champs de la trame 
		switch(etat)
		{
			case 0: // lecture de l'opération et du type
				*op=(((unsigned char)c) & 0xF0) >> 4;
				*type=(((unsigned char)c) & 0x0F);
				etat++; // etape suivante à la prochaine itération
				break;
			case 1: // lecturde de l'adresse de la variable
				*var=(unsigned char)c;
				etat++;
				break;
			case 2: // lecture data poid fort
				*val=c*256;
				etat++;
				break;
			case 3: // lecture data poid faible
				*val=*val+c;
				etat++;
				break;
			case 4: // lecture fin de trame
				if(c!='$')
					return 0; // pas de fin de trame correct ... on oubli tout et on sort
				etat++;
				break;
		}
	}
	return 1;
}


void _comio_envoyer_trame(unsigned char op, unsigned char var, unsigned char type, unsigned int val)
{
	unsigned char op_type;
	
	op_type=(op & 0x0F) << 4;
	op_type=op_type | (type & 0x0F);  
	
	unsigned char pfort=(unsigned char)(val/256);
	unsigned char pfaible=(unsigned char)(val-(unsigned int)(pfort*256));
	
	Serial.write('&');
	Serial.write(op_type);
	Serial.write(var);
	Serial.write(pfort);
	Serial.write(pfaible);
	Serial.write('$');
	Serial.flush();
}


void _comio_envoyer_trame_erreur(unsigned char nerr)
{
	Serial.write('!');
	Serial.write(nerr);
	Serial.write('$');
}


boolean _comio_faire_operation(unsigned char op, unsigned char var, unsigned char type, unsigned int val)
{
	unsigned int retval=0;
	
	if(op==OP_ECRITURE)
	{
		switch(type)
		{
			case TYPE_NUMERIQUE:
				if(!val)
					digitalWrite(var,LOW);
				else
					digitalWrite(var,HIGH);
				break;
			case TYPE_NUMERIQUE_PWM:
				analogWrite(var,val);
				break;
			case TYPE_MEMOIRE:
				comio_mem[var]=(unsigned char)val;
				break;
			default:
				return false;
		}
		retval=val;
	}
	else if (op==OP_LECTURE)
	{
		switch(type)
		{
			case TYPE_ANALOGIQUE:
				retval=analogRead(var);
				break;
			case TYPE_NUMERIQUE:
				retval=digitalRead(var);
				break;
			case TYPE_MEMOIRE:
				retval=comio_mem[var];
				break;
			default:
				return false;
		}
	}
	else if (op==OP_FONCTION)
	{
		if(var<COMIO_MAX_FX)
		{
			if(comio_fonctions[var])
				retval=comio_fonctions[var](val);
		}
	}
	
	_comio_envoyer_trame(op,var,type,retval);
	
	return true;
}


int _comio_valider_operation(unsigned char op, unsigned char var, unsigned char type, unsigned int val)
{
	switch(type)
	{
		case TYPE_NUMERIQUE:
			if(comio_index_N[var]!=op)
				return 10;
			break;
		case TYPE_NUMERIQUE_PWM:
			if(comio_index_N[var]!=op)
				return 11;
			break;
		case TYPE_ANALOGIQUE:
			if(comio_index_A[var]!=op)
				return 12;
			break;
		case TYPE_MEMOIRE:
			if(var>=COMIO_MAX_M)
				return 13;
			break;
		case TYPE_FONCTION:
			if(var>=COMIO_MAX_FX)
				return 14;
			break;
		default:
			return COMIO_ERR_UNKNOWN; // type inconnu
	}     
	return 0;
}


void comio_set_function(unsigned char num_function, callback_f function)
{
	if(num_function<COMIO_MAX_FX)
		comio_fonctions[num_function]=function;
}


void comio_setup(unsigned char io_defs[][3])
{
	// mise à zero des zones mémoires
	memset(comio_index_N,-1,sizeof(comio_index_N));
	memset(comio_index_A,-1,sizeof(comio_index_A));
	memset(comio_fonctions,0,sizeof(comio_fonctions));
	memset(comio_mem,0,sizeof(comio_mem));
	
   if(!io_defs)
      return;
   
	// initialisation des entrées/sorties
	for(int i=0;io_defs[i][0]!=255;i++)
	{
		switch(io_defs[i][2])
		{
			case TYPE_NUMERIQUE:
				comio_index_N[io_defs[i][0]]=io_defs[i][1];
				if(io_defs[i][1]==OP_ECRITURE)
					pinMode((int)io_defs[i][0],OUTPUT);
				else if(io_defs[i][1]==OP_LECTURE)
					pinMode((int)io_defs[i][0],INPUT);
				break;
				
			case TYPE_ANALOGIQUE:
				if(io_defs[i][1]==OP_LECTURE)
				{
					comio_index_A[io_defs[i][0]]=io_defs[i][1];
					pinMode((int)io_defs[i][0],INPUT);
				}
				break;
				
			case TYPE_NUMERIQUE_PWM:
				if(io_defs[i][1]==OP_ECRITURE)
				{
					comio_index_N[io_defs[i][0]]=io_defs[i][1];
					// pinMode(int)io_defs[0],OUTPUT);
					analogWrite((int)io_defs[i][0],0);
				}
				break;
		}
	}
}


void comio_envoyer_trap(unsigned char num_trap)
{
	Serial.write('*');
	Serial.write(num_trap);
	Serial.write('$');
	Serial.flush(); // on s'assure que tout les caractères sont envoyer avant de reprendre la boucle
}


void comio_send_long_trap(unsigned char num_trap, char *value, char l_value)
{
	Serial.write('#');
	Serial.write(num_trap);
   Serial.write(l_value);
   for(int i=0;i<l_value;i++)
      Serial.write(value[i]);
	Serial.write('$');
	Serial.flush(); // on s'assure que tout les caractères sont envoyer avant de reprendre la boucle
}


int comio()
{
	if(!Serial.available())
		return 0;
	
	unsigned char cptr=0;
	boolean flag=false;
	while(Serial.available() && cptr<10) // lecture jusqu'à 10 caractères pour détecter un début de trame
	{
		int c=Serial.read();
		if(c=='?') // un début de trame est détecté
		{
			flag=true;
			break;
		}
		cptr++;
	}
	if(!flag)
		return 0; // pas de début de trame, on rend la main pour ne pas trop bloquer la boucle principale
	
	unsigned char op, var, type;
	unsigned int val;
	if(_comio_lire_trame(&op,&var,&type,&val)) // si on obtient pas une trame on rend la main "sans rien dire"
	{
		int ret=_comio_valider_operation(op,var,type,val);
		if(!ret)
		{
			ret=_comio_faire_operation(op,var,type,val);
		}
		else
		{
			_comio_envoyer_trame_erreur(ret);
			return 1;
		}
	}
	return 0;
}
