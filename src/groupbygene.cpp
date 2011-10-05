/**
 * Author:
 *	Pierre Lindenbaum PhD
 * Contact:
 *	plindenbaum@yahoo.fr
 * Date:
 *	Oct 2011
 * WWW:
 *	http://plindenbaum.blogspot.com
 * Motivation:
 *	group VCF data by y=Gene,x=Sample
 * Compilation:
 *	 g++ -o groupbygene -Wall -O3 groupbygene.cpp -lz
 */
#include <cstdlib>
#include <vector>
#include <map>
#include <set>
#include <cerrno>
#include <string>
#include <cstring>
#include <stdexcept>
#include <climits>
#include <cmath>
#include <cfloat>
#include <cstdio>
#include <iostream>
#include <zlib.h>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <stdint.h>

using namespace std;

#define THROW(a) do{ostringstream _os;\
	_os << __FILE__ << ":"<< __LINE__ << ":"<< a << endl;\
	throw runtime_error(_os.str());\
	}while(0)

typedef int column_t;

#define CHECK_COL_INDEX(idx,tokens) do { if(((size_t)idx)>=tokens.size()) \
	{\
	THROW("Column index out of range ("<< idx << ") in "<< line);\
	} } while(0)
class GroupByGene
    {
    public:

	class SampleInfo
	    {
	    public:
		string sampleName;
		int index;
	    };


	class Mutation
	    {
	    public:
		int pos;
		string ref;
		string alt;
		Mutation(int pos,string ref,string alt):pos(pos),ref(ref),alt(alt)
		    {
		    }
		Mutation(const Mutation& cp):pos(cp.pos),ref(cp.ref),alt(cp.alt)
		    {
		    }
		~Mutation()
		    {
		    }
		Mutation& operator=(const Mutation& cp)
		    {
		    if(this!=&cp)
			{
			pos=cp.pos;
			ref.assign(cp.ref);
			alt.assign(cp.alt);
			}
		    return *this;
		    }

		bool operator==(const Mutation& cp) const
		    {
		    return pos == cp.pos &&
			    strcasecmp(ref.c_str(),cp.ref.c_str())==0 &&
			    strcasecmp(alt.c_str(),cp.alt.c_str())==0
			    ;
		    }

		bool operator<(const Mutation& cp) const
		    {
		    int i= pos- cp.pos;
		    if(i!=0) return i<0;
		    i= strcasecmp(ref.c_str(),cp.ref.c_str());
		    if(i!=0) return i<0;
		    i= strcasecmp(alt.c_str(),cp.alt.c_str());
		    return i<0;
		    }
	    };

	class GeneInfo
	    {
	    public:
		string geneName;
		string chrom;
		int chromStart;
		int chromEnd;
		set<Mutation> mutations;
		vector<int> sample2count;
	    };

	char delim;
	set<string> samples;
	column_t chromcol;
	column_t poscol;
	column_t genecol;
	column_t refcol;
	column_t altcol;
	column_t samplecol;
	map<string,GeneInfo*> gene2info;
	map<string,SampleInfo*> sample2info;
	bool use_ref_alt;
	bool first_line_header;







	GroupByGene()
	    {
	    delim='\t';
	    chromcol=0;
	    poscol=1;
	    refcol=2;
	    altcol=3;
	    genecol=-1;
	    samplecol=-1;
	    use_ref_alt=true;
	    first_line_header=true;
	    }
	~GroupByGene()
	    {
	    for(map<string,SampleInfo*>::iterator r1=sample2info.begin();
		r1!=sample2info.end();
		++r1
		)
		{
		delete r1->second;
		}
	    for(map<string,GeneInfo*>::iterator r2=gene2info.begin();
		r2!=gene2info.end();
		++r2
		)
		{
		delete r2->second;
		}
	    }

	bool readline(gzFile in,std::string& line)
	    {
	    line.clear();
	    int c;
	    if(gzeof(in)) return false;
	    while((c=gzgetc(in))!=EOF && c!='\n')
		    {
		    line+=(char)c;
		    }
	    return true;
	    }
	void split(const string& line,vector<string>& tokens)
	    {
	    size_t prev=0;
	    size_t i=0;
	    tokens.clear();
	    while(i<=line.size())
		{
		if(i==line.size() || line[i]==delim)
		    {
		    tokens.push_back(line.substr(prev,i-prev));
		    if(i==line.size()) break;
		    prev=i+1;
		    }
		++i;
		}
	    }

	void print()
	    {
	    cout << "GENE\tCHROM\tSTART\tEND\tcount(SAMPLES)\tcount(distinct_MUTATION)";
	    for(map<string,SampleInfo*>::iterator r2=sample2info.begin();
		r2!=sample2info.end();
		++r2
		)
		{
		cout << "\tcount(" << r2->first<< ")";
		}
	    cout << endl;

	    for(map<string,GeneInfo*>::iterator r1=gene2info.begin();
		r1!=gene2info.end();
		++r1
		)
		{
		GeneInfo* gene=r1->second;
		size_t count_samples(0);
		for(size_t i=0;i< gene->sample2count.size();++i)
		    {
		    if(gene->sample2count[i]) count_samples++;
		    }
		cout << gene->geneName << "\t"
			<< gene->chrom << "\t"
			<< gene->chromStart << "\t"
			<< gene->chromEnd << "\t"
			<< count_samples << "\t"
			<< gene->mutations.size()
			;
		for(map<string,SampleInfo*>::iterator r2=sample2info.begin();
		    r2!=sample2info.end();
		    ++r2
		    )
		    {
		    cout << "\t";
		    if((size_t)( r2->second->index) >= gene->sample2count.size() )
			{
			cout << "0";
			}
		    else
			{
			cout << gene->sample2count.at((size_t)r2->second->index);
			}
		    }
		cout << endl;
		}

	    }

	void readVCF(gzFile in)
	    {
	    vector<string> header;
	    string line;
	    if(first_line_header)
		{
		if(!readline(in,line)) THROW("Cannot read header");
		split(line,header);
		CHECK_COL_INDEX(chromcol,header);
		CHECK_COL_INDEX(poscol,header);
		if(use_ref_alt) CHECK_COL_INDEX(refcol,header);
		if(use_ref_alt) CHECK_COL_INDEX(altcol,header);
		CHECK_COL_INDEX(genecol,header);
		CHECK_COL_INDEX(samplecol,header);
		}
	    while(readline(in,line))
		{
		if(line.empty()) continue;
		vector<string> tokens;
		split(line,tokens);
		CHECK_COL_INDEX(chromcol,tokens);
		CHECK_COL_INDEX(poscol,tokens);
		if(use_ref_alt) CHECK_COL_INDEX(refcol,tokens);
		if(use_ref_alt) CHECK_COL_INDEX(altcol,tokens);
		CHECK_COL_INDEX(genecol,tokens);
		CHECK_COL_INDEX(samplecol,tokens);

		string sampleName=tokens[samplecol];
		if(sampleName.empty())
		    {
		    cerr << "Sample name empty in "<< line << endl;
		    continue;
		    }
		SampleInfo* sampleInfo;
		map<string,SampleInfo*>::iterator r2=sample2info.find(sampleName);
		if(r2==sample2info.end())
		    {
		    sampleInfo=new SampleInfo;
		    sampleInfo->index=sample2info.size();
		    sampleInfo->sampleName=sampleName;
		    sample2info.insert(make_pair<string,SampleInfo*>(sampleName,sampleInfo));
		    }
		else
		    {
		    sampleInfo=r2->second;
		    }

		char* p2;
		int pos=(int)strtol(tokens[poscol].c_str(),&p2,10);
		if(pos<0 || *p2!=0)
		    {
		    THROW("Bad position in "<< line);
		    }

		string geneName=tokens[genecol];
		if(geneName.empty())
		    {
		    cerr << "Gene name empty in "<< line << endl;
		    continue;
		    }
		GeneInfo* geneInfo=NULL;
		map<string,GeneInfo*>::iterator r1=gene2info.find(geneName);
		if(r1==gene2info.end())
		    {
		    geneInfo=new GeneInfo;
		    geneInfo->geneName=geneName;
		    geneInfo->chrom=tokens[chromcol];
		    geneInfo->chromStart=pos;
		    geneInfo->chromEnd=pos;
		    gene2info.insert(make_pair<string,GeneInfo*>(geneName,geneInfo));
		    }
		else
		    {
		    geneInfo=(*r1).second;
		    if(geneInfo->chrom.compare(tokens[chromcol])!=0)
			{
			THROW("Gene "<< geneName << " defined on two chromosome "<<
			    geneInfo->chrom << " and " << tokens[chromcol]
			    );
			}
		    geneInfo->chromStart=min(geneInfo->chromStart,pos);
		    geneInfo->chromEnd=max(geneInfo->chromEnd,pos);
		    geneInfo=r1->second;
		    }
		geneInfo->sample2count.resize(sampleInfo->index+1,0);
		geneInfo->sample2count.at(sampleInfo->index)++;

		Mutation mut(pos,"","");
		if(use_ref_alt)
		    {
		    mut.ref.assign(tokens[refcol]);
		    mut.alt.assign(tokens[altcol]);
		    }
		geneInfo->mutations.insert(mut);
		}
	    }
    };

#define SETINDEX(option,col) else if(std::strcmp(argv[optind],option)==0 && optind+1<argc) \
	{\
	char* p2;\
	int idx=strtol(argv[++optind],&p2,10);\
      	if(idx<1) THROW("Bad " option " index in "<< argv[optind]);\
	app.col=idx-1;\
	}
#define SHOW_OPT(col) \
	cerr << "  --"<< cols<< " (column index)\n";

static void usage(const char* prg,GroupByGene& app)
	{
	cerr << prg << "Pierre Lindenbaum PHD. 2011.\n";
	cerr << "Compilation: "<<__DATE__<<"  at "<< __TIME__<<".\n";
	cerr << "Options:\n";
	cerr << "  --delim (char) delimiter default:tab\n";
	cerr << "  --norefalt : don't look at REF and ALT\n";
	cerr << "  --sample SAMPLE column index\n";
	cerr << "  --gene GENE column index\n";
	cerr << "  --chrom CHROM column index: default "<< (app.chromcol+1) << "\n";
	cerr << "  --pos POS position column index: default "<< (app.poscol+1) << "\n";
	cerr << "  --ref REF reference allele column index: default "<< (app.refcol+1) << "\n";
	cerr << "  --alt ALT alternate allele column index: default "<< (app.altcol+1) << "\n";
	cerr << "  --no-header first line is NOT header.\n";
	cerr << "(stdin|vcf|vcf.gz)\n";
	}

int main(int argc,char** argv)
    {
    GroupByGene app;
    int optind=1;
    while(optind < argc)
   		{
   		if(std::strcmp(argv[optind],"-h")==0)
   			{
   			usage(argv[0],app);
   			return (EXIT_FAILURE);
   			}
   		SETINDEX("--sample",samplecol)
   		SETINDEX("--chrom",chromcol)
   		SETINDEX("--pos",poscol)
   		SETINDEX("--ref",refcol)
   		SETINDEX("--alt",altcol)
   		SETINDEX("--gene",genecol)
   		else if(std::strcmp(argv[optind],"--no-header")==0)
		    {
		    app.first_line_header=false;
		    }
   		else if(std::strcmp(argv[optind],"--norefalt")==0)
   			{
   			app.use_ref_alt=false;
   			}
   		else if(std::strcmp(argv[optind],"--delim")==0 && optind+1< argc)
			{
			char* p=argv[++optind];
			if(strlen(p)!=1)
			    {
			    cerr << "Bad delimiter \""<< p << "\"\n";
			    usage(argv[0],app);
			    return(EXIT_FAILURE);
			    }
			app.delim=p[0];
			}
   		else if(argv[optind][0]=='-')
   			{
   			fprintf(stderr,"unknown option '%s'\n",argv[optind]);
   			usage(argv[0],app);
   			return (EXIT_FAILURE);
   			}
   		else
   			{
   			break;
   			}
   		++optind;
                }
    if(app.genecol==-1)
	{
	cerr << "Undefined gene column."<< endl;
	usage(argv[0],app);
	return (EXIT_FAILURE);
	}
    if(app.samplecol==-1)
    	{
    	cerr << "Undefined sample column."<< endl;
    	usage(argv[0],app);
    	return (EXIT_FAILURE);
    	}
    if(optind==argc)
	    {
	    gzFile in=gzdopen(fileno(stdin),"r");
	    if(in==NULL)
		{
		cerr << "Cannot open stdin" << endl;
		return EXIT_FAILURE;
		}
	    app.readVCF(in);
	    }
    else
	    {
	    while(optind< argc)
		{
		char* filename=argv[optind++];
		gzFile in=gzopen(filename,"r");
		if(in==NULL)
		    {
		    cerr << "Cannot open "<< filename << " " << strerror(errno) << endl;
		    return EXIT_FAILURE;
		    }
		app.readVCF(in);
		gzclose(in);
		}
	    }
    app.print();
    return EXIT_SUCCESS;
    }
