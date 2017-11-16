#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

struct color {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
};

static unsigned int logo_width;
static unsigned int logo_height;
static struct color **logo_data;

static const char *programname;
static const char *filename;
static const char *logoname = "logo_linux_rgb24";
static const char *outputname;             
static FILE *out;   

static unsigned int get_number(FILE *fp)
{
	int c, val;

	/* Skip leading whitespace */
	do {                                               
		c = fgetc(fp);                                 
		if (c == EOF) {
			fprintf(stderr, "%s: end of file\n", filename);        
			exit(0);
		}
		if (c == '#') {
			/* Ignore comments 'till end of line */
			do {
				c = fgetc(fp);
				if (c == EOF) {
					fprintf(stderr, "%s : end of file\n", filename);
					exit(0);
				}
			} while (c != '\n');
		}
	} while (isspace(c));

	/* Parse decimal number */                         
	val = 0;                                           
	while (isdigit(c)) {                               
		val = 10*val+c-'0';                            
		c = fgetc(fp);                                 
		if (c == EOF) {
			fprintf(stderr, "%s: end of file\n", filename);        
			exit(0);
		}
	}

	return val;
}

static unsigned int get_number255(FILE *fp, unsigned int maxval)
{                       
	unsigned int val = get_number(fp);                          
	return (255*val+maxval/2)/maxval;                           
}       

static void read_image(void)
{
	FILE *fp;
	unsigned int i, j;
	int magic;
	unsigned int maxval;

	/* open image file */
	fp = fopen(filename, "r");
	if (!fp)
		fprintf(stderr, "Cannot open file %s: %s\n",
				filename, strerror(errno));      

	/* check file type and read file header */
	magic = fgetc(fp);
	if (magic != 'P')
		fprintf(stderr, "%s is not a PNM file\n", filename);
	magic = fgetc(fp);
	switch (magic) {
	case '1':
	case '2':
	case '3':
		/* Plain PBM/PGM/PPM */
		break;
	case '4':
	case '5':
	case '6':
		fprintf(stderr, "%s: Binary PNM is not supported\n",
				filename);
		exit(0);
		break;
	}

	logo_width = get_number(fp);
	logo_height = get_number(fp);

	/* allocate image data */                                               
	logo_data = (struct color **)malloc(logo_height*sizeof(struct color *));
	if (!logo_data) {
		fprintf(stderr, "%s\n", strerror(errno));
		exit(0);
	}
	for (i = 0; i < logo_height; i++) {
		logo_data[i] = malloc(logo_width*sizeof(struct color));             
		if (!logo_data[i]) {
			fprintf(stderr, "%s\n", strerror(errno));
			exit(0);
		}
	}

	switch (magic) {
	case '3':
		maxval = get_number(fp);                                  
		printf ("maxval = %d\n", maxval);
		for (i = 0; i < logo_height; i++)                         
			for (j = 0; j < logo_width; j++) {                    
				logo_data[i][j].red = get_number255(fp, maxval);  
				logo_data[i][j].green = get_number255(fp, maxval);
				logo_data[i][j].blue = get_number255(fp, maxval); 
			}                                                     

		break;
	default:
		fprintf(stderr, "P%c : unssuported\n", magic);
		exit(0);
		break;
	}
}

static void write_header(void)
{
	/* open logo file */
	if (outputname) {
		out = fopen(outputname, "w");
		if (!out) {
			fprintf(stderr, "Cannot create file %s: %s\n",
					outputname, strerror(errno));
			exit(0);
		}
	} else {                                                                
		out = stdout;                                                       
	}                                                                       

	fputs("/*\n", out);
	fprintf(out, " *  It was automatically generated from %s\n", filename); 
	fputs(" *\n", out);                                                     
	fprintf(out, " *  Linux logo %s\n", logoname);
	fputs(" */\n\n", out);
	fputs("#include <linux/linux_logo.h>\n\n", out);
	fprintf(out, "static unsigned char %s_data[] __initdata = {\n",
			logoname);
}

static int write_hex_cnt;
static void write_hex(unsigned char byte)
{       
	if (write_hex_cnt % 12)
		fprintf(out, ", 0x%02x", byte);   
	else if (write_hex_cnt) 
		fprintf(out, ",\n\t0x%02x", byte);
	else
		fprintf(out, "\t0x%02x", byte);   
	write_hex_cnt++;
}

static void write_footer(void)                                             
{                                                                          
	fputs("\n};\n\n", out);
	fprintf(out, "const struct linux_logo %s __initconst = {\n", logoname);
	fprintf(out, "\t.type\t\t= LINUX_LOGO_CLUT224,\n");//, logo_types[logo_type]);
	fprintf(out, "\t.width\t\t= %d,\n", logo_width);                       
	fprintf(out, "\t.height\t\t= %d,\n", logo_height);                     
	if (1){//logo_type == LINUX_LOGO_CLUT224) {
		//fprintf(out, "\t.clutsize\t= %d,\n", logo_clutsize);
		//fprintf(out, "\t.clut\t\t= %s_clut,\n", logoname);
		fprintf(out, "\t.clutsize\t= 0,\n");
		fprintf(out, "\t.clut\t\t= %s_clut,\n", logoname);
	}
	fprintf(out, "\t.data\t\t= %s_data\n", logoname);     
	fputs("};\n\n", out);

	/* close logo file */                                 
	if (outputname)
		fclose(out);
}

int main(int argc, char **argv)
{
	int i, j, k;

        if (argc <= 2 || ((argc == 2) && (!strcmp(argv[1], "--help")))) {
		fprintf(stdout, "%s filename outname\n", argv[0]);
		exit(0);
	}

	programname = argv[0];
	filename = argv[1];
	outputname = argv[2];

	read_image();

	write_header();

	for (i = 0; i < logo_height; i++)
		for (j = 0; j < logo_width; j++) {                         
			int value;
			value = (logo_data[i][j].blue & 0xF8) << 8; // >> 3
			value |= (logo_data[i][j].green & 0xFC) << 3; // >> 2
			value |= (logo_data[i][j].red & 0xF8) >> 3;
			/*printf("%x %x %0x, %x\n", 
					logo_data[i][j].red,
					logo_data[i][j].green,
					logo_data[i][j].blue, value);*/
			write_hex((unsigned char)(value & 0xFF));
			write_hex((unsigned char)((value & 0xFF00) >> 8));
//			write_hex(logo_data[i][j].red);
//			write_hex(logo_data[i][j].green);
//			write_hex(logo_data[i][j].blue);
		}
	fputs("\n};\n\n", out);

	/* write logo clut */                                          
	fprintf(out, "static unsigned char %s_clut[] __initdata = {\n",
			logoname);                                             
	write_footer();

	return 0;
}
