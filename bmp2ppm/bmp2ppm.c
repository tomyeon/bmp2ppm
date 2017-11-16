#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct data_info {
	int w;
	int h;
	int size;
	int comp;
	int *data;
};

struct bmpheader {
	int size;
	int width;
	int height;
	int planes;
	int bitcount;
	int comp;
	int sizeimage;
	int xpelspermeter;
	int ypelspermeter;
	int clrused;
	int clrimportant;
};

const char *prog_name;

void print_help(void);

char read_raw1byte(FILE *p);
int read_raw2byte(FILE *p);
int switch_le2int(int value);
int read_2byte(FILE *p);
int read_4byte(FILE *p);

struct data_info * load_bmpfile(const char *filename);
void write_ppmfile(struct data_info *raw_data, const char *filename);

int main(int argc, char **argv)
{
	prog_name = argv[0];
	struct data_info *raw_data;
	if (argc < 3 || !strcmp(argv[1], "--help")
				|| !strcmp(argv[1], "-h"))
		print_help();

	raw_data = load_bmpfile(argv[1]);

	if (raw_data) {
		write_ppmfile(raw_data, argv[2]);

		if (raw_data) {
			if (raw_data->data)
				free(raw_data->data);
			free(raw_data);
		}
	}

	return 0;
}

void print_help(void)
{
	fprintf(stdout, "%s bmpfile ppmfile\n", prog_name);
	exit(0);
}

char read_raw1byte(FILE *p)
{
	char v;

	v = fgetc(p);
	printf("0x%02X\n", v);

	return v;
}

int read_raw2byte(FILE *p)
{
	int val;

	val = (fgetc(p) & 0xFF) << 8;
	val |= (fgetc(p) & 0xFF);

	return val;
}

int switch_le2int(int value)
{
	int val;

	val = ((value & 0xFF) << 8) & 0xFF00;
	val |= ((value & 0xFF00) >> 8);

	return val;
}

int read_2byte(FILE *p)
{
	int val;

	val = read_raw2byte(p);
	return switch_le2int(val);
}

int read_4byte(FILE *p)
{
	int val1, val2;
	val1 = read_2byte(p);
	val2 = read_2byte(p);
	val2 = (val2 << 16) & 0xFFFF0000;
	val2 |= (val1 & 0xFFFF);

	return val2;
}

struct data_info * load_bmpfile(const char *filename)
{
	int val, count, offset;
	FILE *p = NULL;
	struct data_info *raw_data = NULL;
	struct bmpheader header;

	p = fopen(filename, "r");
	if (!p) {
		fprintf(stderr, "failed to open %s\n", filename);
		goto _load_bmpfile_err0;
	}

	raw_data = malloc(sizeof(struct data_info));
	memset(raw_data, 0, sizeof(struct data_info));

	/* check marker */	
	val = read_raw2byte(p);
	if (val != 0x424D) {
		fprintf(stderr, "this is not bmp file\n");
		goto _load_bmpfile_err1;
	}

	/* read size */ 
	raw_data->size = read_4byte(p);
	printf ("size:%d\n", raw_data->size);

	/* dummy */
	read_raw2byte(p);
	read_raw2byte(p);

	/* read data offset */
	offset = read_4byte(p);
	printf ("offset:%d\n", offset);

	/* header */
	memset(&header, 0, sizeof(header));

	header.size = read_4byte(p);
	raw_data->size -= (header.size + 14);
	header.width = read_4byte(p);
	header.height = read_4byte(p);
	header.planes = read_2byte(p);
	header.bitcount = read_2byte(p);
	raw_data->comp = header.comp = read_4byte(p);
	header.sizeimage = read_4byte(p);
	header.xpelspermeter = read_4byte(p);
	header.ypelspermeter = read_4byte(p);
	header.clrused = read_4byte(p);
	header.clrimportant = read_4byte(p);

	printf ("size:%d, header size:%d\n", raw_data->size, header.size);
	printf("width:%d, height:%d, planes:%d, bitcount:%d\n",
			header.width, header.height,
			header.planes, header.bitcount);
	printf("comp:%d, sizeimage:%d, xpelspermeter:%d, ypelspermeter:%d\n",
			header.comp, header.sizeimage,
			header.xpelspermeter, header.ypelspermeter);
	printf("clrused:%d, clrimportant:%d\n",
			header.clrused, header.clrimportant);

	if (header.width <= 0 ||
			header.height <= 0 ||
			header.bitcount != 16) {
		fprintf(stdout, "invalid format, w:%d, h:%d, bitcount:%d",
				header.width, header.height, header.bitcount);
		goto _load_bmpfile_err1;
	}

	if (!(header.comp == 0 ||
		header.comp == 3)) {
		fprintf(stdout, "invalid comp:%d, comp should be 0 or 3\n",
				header.comp);
		goto _load_bmpfile_err1;
	}

	raw_data->w = header.width;
	raw_data->h = header.height;
	raw_data->data = (int*)malloc(sizeof(int) * raw_data->size);
	memset(raw_data->data, 0, sizeof(int) * raw_data->size);

	/* load data */
	count = 0;
	fseek(p, offset, SEEK_SET);
	while ((val = fgetc(p)) != EOF) {
		raw_data->data[count++] = (int)(val & 0xFF);
	}
	printf("count:%d\n", count);
	
	goto _load_bmpfile_err0;

_load_bmpfile_err1:
	if (raw_data) {
		if (raw_data->data)
			free(raw_data->data);
		free(raw_data);
	}
	raw_data = NULL;
	
_load_bmpfile_err0:
	if (p) 
		fclose(p);

	return raw_data;
}

int write_rgb(int r, int g, int b, FILE *p)
{
	int ret;
	char buff[128] = {0,};
	
	ret = fprintf(p, "%d ", r<=255?r:255);
	if (ret < 0) return ret;
	ret = fprintf(p, "%d ", g<=255?g:255);
	if (ret < 0) return ret;
	ret = fprintf(p, "%d ", b<=255?b:255);
	if (ret < 0) return ret;
		
	return 0;
}

void write_ppmfile(struct data_info *raw_data, const char *filename)
{
	int i, j, k;
	FILE *p = NULL;

	if (!raw_data || !filename) {
		fprintf(stderr, "invalid parameter\n");
		goto _write_ppmfile_err0;
	}

	p = fopen(filename, "w");
	if (!p) {
		fprintf(stderr, "failed to open %s\n", filename);
		goto _write_ppmfile_err0;
	}

	/* init */
	fprintf(p, "P3\n");
	fprintf(p, "# create ppm file\n");
	fprintf(p, "%d %d\n", raw_data->w, raw_data->h);
	fprintf(p, "255\n");

	k = 0;
	for (i = 0; i < raw_data->h; i++) {
		for (j = 0; j < raw_data->w; j++) {
			int rgb, val1, val2, pos;
			int r, g, b;

			pos = (raw_data->h - i - 1) * raw_data->w * 2;
			val1 = (int)(raw_data->data[pos + j*2 + 0]) & 0xFF;
			val2 = (int)(raw_data->data[pos + j*2 + 1]) & 0xFF;

			rgb = (val2 << 8) & 0xFF00;
			rgb |= (val1 & 0x00FF);

			if (raw_data->comp == 0) {
				r = (int)(((rgb >> 10) << 3) & 0x0F8);
				g = (int)(((rgb >> 5) << 3) & 0x0F8);
				b = (int)((rgb << 3) & 0x0F8);
			} else if (raw_data->comp == 3) {
				r = (int)(((rgb >> 11) << 3) & 0x0F8);
				g = (int)(((rgb >> 5) << 2) & 0x0FC);
				b = (int)((rgb << 3) & 0x0F8);
			}

			if (write_rgb(r, g, b, p) < 0) {
				fprintf(stderr, "write error\n");
				goto _write_ppmfile_err0;
			}

			k++;

			if (k != 0 && (k % 6 == 0))
				fprintf(p, "\n");
		}
	}
_write_ppmfile_err0:
	if (p) 
		fclose(p);
}
