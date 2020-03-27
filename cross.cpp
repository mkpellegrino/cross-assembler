#include <algorithm>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <cctype>
#include <math.h>
#include <cstdlib>
#include <locale>  

// included for the parser
#include <sstream>
#include <string>


using namespace std;

vector<unsigned char> byte_vector;

int error_count=0;
int compilation_failed=0;
int memorylocation=0;
int start_counting=0;
int program_index=0;
int function_ui=0;
int store_op1=0;
int disp_op1=0;
int convop1b=0;
int fully_clear_screen=0;
string name;
int line_number=0;

int add_input=0;
int add_store_op1=0;
int add_disp_op1=0;
int add_convop1b=0;
int add_fully_clear_screen=0;
class offset;
class label;
class mnemonic;
void a(string s);
void sysCall(string s);
void addByte(unsigned char);

vector<label> label_vector;
vector<label> label_cr_vector;
vector<offset> offset_vector;

vector<mnemonic> mnemonic_vector;

class mnemonic
{
public:
  mnemonic( unsigned int c )
  {
    byte0=c;
  };
  void setText(string s){ text=s; };
  void setSize(int s){ size=s; };
  void setByte(unsigned int b){ byte0=b; };
  unsigned int getByte(){ return byte0; };
  string getText(){ return text; };
private:
  unsigned int byte0;
  string text;
  int size;
  friend ostream &operator << (ostream &out, const mnemonic &m);   
};

ostream & operator << (ostream &out, const mnemonic &m) 
{
  out << m.byte0;
  return out; 
}

class label
{
public:
  label( string s, int a )
  {
    index=program_index;
    memory_location=a;
    label_number=-1;
    name=s;
    line_num=line_number;
    
  }
  label( string s )
  {
    index=program_index;
    memory_location=memorylocation;
    label_number=-1;
    name=s;
    line_num=line_number;
  }
  label()
  {
    index=program_index;
    memory_location=memorylocation;
    label_number=-1;
    line_num=line_number;
    name="";
  };
  label(int l)
  {
    index=program_index;
    memory_location=memorylocation;
    label_number=l;
    line_num=line_number;
    name="";
  };
  ~label(){};
  int getIndex(){ return index; };
  int getMemoryLocation(){ return memory_location; };
  int getLabelNumber(){ return label_number; };
  string getName(){ return name; };
  void process()
  {
    if( label_number==-1 )
      {
	// search by name instead
	int ml=-1;
	for( int i=0; i<label_cr_vector.size(); i++ )
	  {
	    
	    if( label_cr_vector[i].getName() == name )
	      {
		ml=label_cr_vector[i].getMemoryLocation();
	      }	    
	  }
	if( ml == -1 )
	  {
	    // Label wasn't found
	    cerr << endl << "*** label wasn't found [" << name << "] - compilation failed ***" << endl;
	    cerr << "    line number: " << dec << line_number << endl;

	    cerr << "    program index: 0x" << hex << uppercase << program_index << " (" << dec << program_index << " decimal)" << endl << endl;
	    compilation_failed=1;
	    error_count++;
	  }

	// OPTIMIZE HERE
	
	{
	  unsigned char c = byte_vector[index-1];
	  if(  ((c==0xC3)||(c==0xC2)||(c==0xD2)||(c==0xE2)||(c==0xF2)||(c==0xCA)||(c==0xDA)||(c==0xEA)||(c==0xFA)||(c==0xE9)) && (abs(ml-memory_location)<126) )
	    {
	      cerr << "*** jump [" << index-1 << "] could be made relative...  distance is: " << dec << abs( ml-memory_location )<< " [" << name << "]\nLine Number: " << dec << line_num << endl;
	    }

	}
	byte_vector[index]=ml&0xFF;
	byte_vector[index+1]=(ml&0xFF00)/0xFF;
      }
    else
      {
#ifdef DEBUG
	cerr << "changing bytes: [" << index << " and " << index+1 << "] to ["
	     << hex << (label_cr_vector[label_number-1].getMemoryLocation()&0xFF) << " and "
	     << hex << ( (label_cr_vector[label_number-1].getMemoryLocation()&0xFF00 )/0xFF) << "]" <<  endl;
#endif 
	byte_vector[index] = (label_cr_vector[label_number-1].getMemoryLocation()&0xFF);// L
	byte_vector[index+1] = ((label_cr_vector[label_number-1].getMemoryLocation()&0xFF00)/0xFF );// H
      }
  };
private:
  int index;
  int memory_location;
  int label_number;
  int line_num;
  string name;
};



class offset
{
public:
  offset(){};
  offset(string  s)
  {
    from=program_index-1;
    offset_name=s;
    label_number=-1;
    line_num=line_number;;
#ifdef DEBUG
    cerr << "creating an offset to label: " << s << " from prog index " << program_index << " (Line Number: " << dec << line_num << ")" << endl;
#endif
  };
  
  offset(int i)
  {
    from=program_index-1;
    label_number=i;
    line_num=line_number;
#ifdef DEBUG
    cerr << "creating an offset to label: " << label_number << " from prog index " << program_index << " (Line Number: " << dec << line_num << ")" << endl;
#endif
  };
  ~offset(){};

  void process()
  {
    if( label_number == -1 )
      {
	
	// search by name
	for( int i=0; i<label_cr_vector.size(); i++ )
	  {
	    if(label_cr_vector[i].getName() == offset_name)
	      {
		label_number=i+1;
	      }
	  }

      }
    to=label_cr_vector[label_number-1].getIndex();
    diff=to-from-2;
#ifdef DEBUG
    cerr << endl << "offset -> label [" << dec << from << " -> " << to << "]\tdiff: " << diff << endl << endl;
#endif
    
    if( (diff>126) || (diff<-126) )
      {
	cerr << endl << "*** relative jump is too far - compilation failed ***" << endl;
	cerr << "    line number: " << dec << line_number << endl;
	cerr << "    program index: 0x" << hex << uppercase << from << " (" << dec << from << " decimal)" << " to [" << offset_name << "]" << endl << endl;
	compilation_failed=1;
	error_count++;
      }
    byte_vector[from+1]=(diff&0xFF);// Actually change the code
#ifdef DEBUG
    cerr << "processing offset [diff=" << hex << (unsigned int) (diff&0xFF) << " hex][" << dec << (diff&0xFF) << " dec](" << offset_name << ")" << endl;
#endif
    
  };
private:
  //int index;// Where this is in the code.
  string offset_name;
  int label_number;// Which label number i am pointing to
  int from;// Where this is in the code.
  int to;//Where are we going?
  int line_num;
  signed int diff;//How far is it to there?
};

void startCounting()
{
  start_counting=1;
}

void setMemoryStart(int i)
{
  memorylocation=i;
}

void addLabel( string s, int addr )
{
#ifdef DEBUG
  cerr <<  "[" << hex << program_index << "] creating label at [" << hex << memorylocation << "] called [" << s << "]" << endl;
#endif
  label l=label(s,addr);
  // *******

  label_cr_vector.push_back( l );
  
}
void addLabel( string s )
{
#ifdef DEBUG
  cerr <<  "[" << hex << program_index << "] creating label at [" << hex << memorylocation << "] called [" << s << "]" << endl;
#endif
  
  
  label l=label(s);
  label_cr_vector.push_back( l );
}

void addLabel()
{
#ifdef DEBUG
  cerr << "[" << hex << program_index << " creating label at [" << hex << memorylocation << "] with no name" << endl;
#endif

  label l = label();
  label_cr_vector.push_back( l );
}

void subtractByte()
{
#ifdef DEBUG
  
#endif
  program_index--;
  if( start_counting == 1) memorylocation--;
  byte_vector.erase(byte_vector.begin()+byte_vector.size()-1);
}

void addByte( unsigned char b )
{
#ifdef DEBUG
  cerr << "{" << hex << (unsigned int) b << "}" << endl;
#endif
  program_index++;
  if( start_counting == 1) memorylocation++;  
  byte_vector.push_back( (unsigned char) b );
}

void addOffset(string s)
{
  offset o = offset(s);
  offset_vector.push_back(o);
  addByte(0x00);  
}
void addOffset(int i)
{
  offset o = offset(i);
  offset_vector.push_back(o);
  addByte(0x00);
}

void addWord( int w=0x0000 )
{
#ifdef DEBUG
  cerr << "adding word " << hex << w << "= 0x" << (w&0x00FF) << ":" << ((w&0xFF00)>>8) << endl;
#endif 
  addByte( (unsigned int)w&0xFF );
  addByte( (unsigned int)((w&0xFF00)>>8) );
}

void addAddress( string s )
{
#ifdef DEBUG
  cerr << "creating placeholder at [" << hex << memorylocation << "][" << program_index << "] for [" << s << "]" << endl;
#endif

  label l = label(s);
  label_vector.push_back( l );
  addWord(0x0000);
}

void addAddress( int i )
{
#ifdef DEBUG
  cerr << "creating placeholder at [" << hex << memorylocation << "][" << program_index << "] for label #[" << i << "]" << endl;
#endif
  label l = label(i);
  label_vector.push_back( l );
  addWord(0x0000);
}

void addString( string s )
{
  for( int i=0; i<s.size(); i++ ) addByte( s[i] );
  if( start_counting == 1 ) addByte( 0x00 );
}

void setComment( string s )
{
  s.resize(42,0x00);
  addString(s);
  
}
void setName( string s )
{
  if( name == "" )
    {
      s.resize(8,0x00);
      name = s;
      addString(s);
    }
  else
    {
      name.resize(8,0x00);
      for( int i=0; i<8; i++ ) name[i]=toupper(name[i]);
      addString(name);
    }
}

// THIS IS WHAT PROCESSED EACH MNEMONIC
void a( string s )
{
  // look for "s" in the vector of mnemonics
  // add the byte that is associated with it.
  int found=-1;
  for( int i=0; i<mnemonic_vector.size(); i++ )
    {
      if( s == mnemonic_vector[i].getText() )
	{
	  found=mnemonic_vector[i].getByte();
	  i=mnemonic_vector.size();
	}
    }
  if( found !=-1 )
    {
      addByte(found);
    }
  else
    {
      cerr << endl << "*** unknown instruction [" << s << "] - compilation failed ***" << endl;
      cerr << "    line number: " << dec << line_number << endl;
      cerr << "    program index: 0x" << hex << uppercase << program_index << " (" << dec << program_index << " decimal)" << endl << endl;
      compilation_failed=1;
      error_count++;
    }

}

int relativeJump(int i)
{
#ifdef DEBUG
  cerr << "relative jump: " << label_cr_vector[i-1].getMemoryLocation() - memorylocation << " bytes" << endl;
#endif
  return ( label_cr_vector[i-1].getMemoryLocation() - memorylocation -2);
}

string getBetween( string s, char L='(', char R=')' )
{
  int P1=0;
  int P2=0;
  string retVal;
  // returns the string that's between the ()'s
  for( int i=0; i<s.length(); i++ )
    {
      if( s[i] == L ) P1=i;
      if( s[i] == R ) P2=i;
    }
  P1++;
  if( P1 == 0 || P2== 0)
    {
      retVal=s;
    }
  else
    {
      retVal=s.substr(P1,P2);
      retVal.pop_back();
    }
  return retVal;
}


int stringToHexValue( string s )
{
  int e=0;
  int intg=0;
  int retVal=0;
  for( int i=s.length()-1; i>1; i-- )
    {
      switch( s[i] )
	{
	case '0':
	  intg=0;
	  break;
	case '1':
	  intg=1;
	  break;
	case '2':
	  intg=2;
	  break;
	case '3':
	  intg=3;
	  break;
	case '4':
	  intg=4;
	  break;
	case '5':
	  intg=5;
	  break;
	case '6':
	  intg=6;
	  break;
	case '7':
	  intg=7;
	  break;
	case '8':
	  intg=8;
	  break;
	case '9':
	  intg=9;
	  break;
	case 'A':
	  intg=10;
	  break;
	case 'B':
	  intg=11;
	  break;
	case 'C':
	  intg=12;
	  break;
	case 'D':
	  intg=13;
	  break;
	case 'E':
	  intg=14;
	  break;
	case 'F':
	  intg=15;
	  break;
	default:
	  break;
	}
      retVal+=intg*pow(16,e);
      e++;
    }
  return retVal;
}

   
int getType(string s)
{
  int retVal=16;
  // 0= offset
  // 1= label
  // 2= word
  // 4= byte
  // 8= character
  // 16 = unknown
  
  if( s[0] == '0' && s[1] == 'x' && s.length() == 4 ) retVal=4;
  //else if( s[0] == '#' && s[1] == '$' && s.length() == 4 ) retVal=4;
  else if( s[0] == '0' && s[1] == 'X' && s.length() == 4 ) retVal=4;
  else if( s[0] == '0' && s[1] == 'x' && s.length() == 6 ) retVal=2;
  else if( s[0] == '0' && s[1] == 'X' && s.length() == 6 ) retVal=2;
  else if( s[0] == '\'' && s[2] == '\'' && s.length()==3 ) retVal=8; 
  else if( s.length() == 1 ) retVal=8;
  return retVal;
}

int stringToValue(string s)
{
  return stoi(s);
}

string removeUnwanted( string s )
{
  while( s[0] == ' ' )
    {
      s=s.substr(1,s.length() );
    }
  while( s.back() == ' ')
    {
      s.pop_back();
    }
  s.erase( remove(s.begin(),s.end(),'\t'),s.end());
  return s;
}

int main(int argc, char *argv[])
{
  // usage:
  // cross -t 6301 input.asm output.bin
  
  if( argc < 5 )
    {
      cerr << "usage: cross -t 6301 input.asm output.bin" << endl;
      exit(-1);
    }
  if( argv[3] == argv[4] )
    {
      cerr << "you must supply an input and an output filename THAT HAS A DIFFERENT NAME" << endl;
      cerr << "example: cross -t 6301 input.asm output.bin" << endl;
      exit(-1);
    }

#ifdef DEBUG
  cerr << "Attempting to compile: " << argv[3] << " into " << argv[4] << " for the " << argv[2] << " processor." <<endl;
#endif

  
  // First read in the cross-chart
  string opcode_fn = string(argv[2]);
  opcode_fn+=string(".op");
  
  ifstream mnemonics(opcode_fn);
  string cross_line;
  if( mnemonics.is_open())
    {
      while (getline(mnemonics, cross_line))
	{
	  // cross_line is my string
	  // now build the mnemonic
	  istringstream ss( cross_line );	  
	  string s;
	  getline( ss, s, ':' );
	  mnemonic tmp_mnemonic(stoi(s));
	  getline( ss, s, ':' );
	  tmp_mnemonic.setSize(stoi(s));
	  getline( ss, s, ':' );
	  tmp_mnemonic.setText(s);

	  mnemonic_vector.push_back(tmp_mnemonic);
	  
	}
    }
  
  ifstream file(argv[3]);  // source code
  FILE *binary = fopen(argv[4], "wb");
  
  startCounting();




  
  // ================================================================================================================================================================================================


  string line;
  if (file.is_open())
    {
      while (getline(file, line))
	{
	  

	  // convert the line to ALL CAPS
	  locale loc;
	  for (string::size_type i=0; i<line.length(); ++i)
	    {
	      line[i]=toupper(line[i], loc );
	    }

	  
	  line_number++;
	  // remove tabs and other unwanted characters
	  line=removeUnwanted(line);
	    
	  // 
	  if( line=="" || line[0]==';' ){}
	  else if( line.substr(0,5) == ".NAME" )
	    {
	      for( int i=0; i<8; i++ )
		{
		  name[i]=toupper(line.substr(6, line.size()-6)[i]);
		  byte_vector[i+60]=name[i];
		}
	    }
	  else if( line.back() == ':' )
	    {
	      // Then we have a label
	      line.pop_back();
	      addLabel(line);
	    }
	  else if( line[0] == '.' )
	    {
#ifdef DEBUG
	      cerr << "directive: [" << line << "]" << endl;
#endif	     
	      // Then we have a directive
	      
	      if( line.substr(0,3)==".DB" || line.substr(0,3)==".dw" )
		{
		  // turn the rest into a number or numbers
		  string tmp = removeUnwanted(line.substr(4, line.length() ));
#ifdef DEBUG
		  cerr << "[data: " << tmp << "]" << endl;
		  cerr << "[type: " << getType(tmp) << "]" << endl;
#endif	      
		  if( getType(tmp) == 4 )
		    {
		      addByte( stringToHexValue(tmp) );
		    }
		  else if( getType(tmp) == 2)
		    {
		      addWord( stringToHexValue(tmp) );
		    }
		  else if( getType(tmp) == 8)
		    {
		      addByte( (int) tmp[1]);
		    }
		  else
		    {
		      cerr << "error: unknown type of data." << endl;
		    }
		}
	      if( line.substr(0,4) ==".STR" )
		{
		  string tmp = removeUnwanted(line.substr(5, line.length() ));		  
#ifdef DEBUG
		  cerr << "[string: " << tmp << "]" << endl;
#endif	      
		  // remove quotes
		  tmp.erase( remove(tmp.begin(),tmp.end(),'\"'),tmp.end());

		  addString( tmp );
		}
	      if( line.substr(0,4) ==".ORG" )
		{
		  string tmp = removeUnwanted(line.substr(5, line.length() ));		  
#ifdef DEBUG
		  cerr << "[memory start: " << stringToHexValue(tmp) << "]" << endl;
#endif	      
		  // remove quotes
		  //tmp.erase( remove(tmp.begin(),tmp.end(),'\"'),tmp.end());

		  // goback
		  setMemoryStart(stringToHexValue(tmp));
		}
	    }
	  else
	    {
	      int processed=0;

	      // & Address Label
	      // % Address Offset
	      // # Word
	      // @ Byte
	      // Z Zeropage
	      
	      // here we have true assembler
	      // find value if there is one
	      // save the value (either a byte or a word)
	      // replace the value with a * for byte or
	      // a ** for word.
	      // that's the mnemonic
	      // then determine if the byte or word is
	      // a direct value, an offset, or an address.
	      size_t found;
	      
	      found = line.find('&');
	      // EXTRACT ADDRESS LABEL ================
	      if (found!=string::npos)
		{
		  int k=-1; string s = line.substr(found,line.length());
		  for( int i=0; i<s.length();i++ ){if( s[i]==' ' || s[i]==')' || s[i]==','){k=i;i=s.length();}}
		  if( k!=-1 ) s=s.substr(0,k);    
		  // Now replace the text with ** replace(9,5,str2);
		  line.replace( found, s.length(), string("**") );
#ifdef DEBUG
		  cerr << endl;
		  cerr << "line of code: " << line << "\taddress: " << s << endl;
		  cerr << endl;
#endif
		  a( line ); processed=1;	  
		  addAddress( s.substr(1,s.length()) );
		}

	      found = line.find('%');
	      if (found!=string::npos)
		{
		  int k=-1; string s = line.substr(found,line.length());
		  for( int i=0; i<s.length();i++ ){if( s[i]==' ' || s[i]==')' || s[i]==','){k=i;i=s.length();}}
		  if( k!=-1 ) s=s.substr(0,k);    
		  // Now replace the text with ** replace(9,5,str2);
		  line.replace( found, s.length(), string("*") );
#ifdef DEBUG
		  cerr << "line of code: " << line << "\toffset: " << s << endl;
#endif
		  a( line );  processed=1;	  
		  addOffset( s.substr(1,s.length() ));
		}

	      found = line.find("#$");
	      if (found!=string::npos)
		{
		  int k=-1; string s = line.substr(found,line.length());
		  for( int i=0; i<s.length();i++ ){if( s[i]==' ' || s[i]==')' || s[i]==','){k=i;i=s.length();}}
		  if( k!=-1 ) s=s.substr(0,k);    
		  line.replace( found, s.length(), string("*") );
#ifdef DEBUG
		  cerr << "line of code: " << line << "\tZero Page: " << s << endl;
#endif
		  a( line );  processed=1;
		  addByte( stringToHexValue(s.substr(1,s.length())) );
		}

	      
	      found = line.find('#');
	      if (found!=string::npos)
		{
		  int k=-1; string s = line.substr(found,line.length());
		  for( int i=0; i<s.length();i++ ){if( s[i]==' ' || s[i]==')' || s[i]==','){k=i;i=s.length();}}
		  if( k!=-1 ) s=s.substr(0,k);    

		  line.replace( found, s.length(), string("**") );
#ifdef DEBUG
		  cerr << "line of code: " << line << "\tWord: " << s << endl;
#endif
		  a( line );  processed=1;
		  addWord( stringToHexValue(s.substr(1,s.length())) );
		}

	      found = line.find('$');
	      if (found!=string::npos)
		{
		  int k=-1; string s = line.substr(found,line.length());
		  for( int i=0; i<s.length();i++ ){if( s[i]==' ' || s[i]==')' || s[i]==','){k=i;i=s.length();}}
		  if( k!=-1 ) s=s.substr(0,k);    
		  // Now replace the text with ** replace(9,5,str2);
		  line.replace( found, s.length(), string("**") );
#ifdef DEBUG
		  cerr << "line of code: " << line << "\tWord: " << s << endl;
#endif
		  a( line );  processed=1;
		  addWord( stringToHexValue(s.substr(1,s.length())) );
		}

	      found = line.find('@');
	      if (found!=string::npos)
		{
		  int k=-1; string s = line.substr(found,line.length());
		  for( int i=0; i<s.length();i++ ){if( s[i]==' ' || s[i]==')' || s[i]==','){k=i;i=s.length();}}
		  if( k!=-1 ) s=s.substr(0,k);    
		  // Now replace the text with ** replace(9,5,str2);
		  line.replace( found, s.length(), string("*") );
#ifdef DEBUG
		  cerr << "FOUND @ : line of code: " << line << "\tByte: " << dec << stringToHexValue(s) << endl;
#endif
		  a( line );  processed=1;
		  if( getType(s) == 4 )
		    {
		      // *****
		      addByte( stringToHexValue(s));
		    }
		  else
		    {
		      addByte(stringToHexValue(s));
		      //addByte( (int) s.substr(1,s.length())[0] );
		    }
		}

	      if( !processed ) a( line );
	      
	    }
	  
	}

      file.close();
    }

  // ================================================================================================================================================================================================

  
  // redo labels
  for( int i=0; i<label_vector.size(); i++ )label_vector[i].process();

  // process offsets
  for( int i=0; i<offset_vector.size(); i++ )offset_vector[i].process();
  
  if( compilation_failed )
    {
      cerr << "Error Count: " << dec << error_count << endl;
      fclose(binary);
      exit(-1);
    }



  // this is where is does the file write
    
  for( int i=0; i<byte_vector.size(); i++ )
    {
      unsigned char byte_to_write = byte_vector[i];
      fwrite(&byte_to_write, 1, sizeof(byte_to_write), binary);
    }

  cerr << endl << "*** Compilation success ***" << endl << endl;
  cerr << "    Filename: " << argv[3] << endl;
  cerr << "        Name: " << argv[4] << endl;    
  cerr << "        Size: " << dec << byte_vector.size() << " bytes" << endl << endl;
  fclose( binary );
  return 0;
}
