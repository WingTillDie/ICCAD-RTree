#include <stdio.h>
#include "rtree.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "DummyInsert.h"

//#define design_transform
//#define svg
#define	svglayer	8
#define critical_expand_factor  6
#define	alpha	-0.15

static  RTREEMBR chip_boundary;
static  double fill_id = 0;

extern double window_width;
extern double min_width[20];
extern double min_space[20];
extern double max_fill_width[20];
extern double min_density[20];
extern double max_density[20];
extern int total_layer;

template <typename T>
struct frame{
    T data;
    struct frame<T> *next;
};

template <typename T>
struct stack {
    frame<T>* head = NULL;
    void push(T data_in){
        frame<T> *node = new frame<T>;
        node->data=data_in;
        node->next=head;
        head=node;
    }
    frame<T>* pop(){//TODO NULL handling //Return pointer? //Pop frame out?
        if(head != NULL){
            frame<T> *out = head;
            head = head->next;
            return out;
        } else {
            return NULL;
        }
    }
};

typedef RTREEMBR DRC_ERRMBR;

/*
typedef struct DRC_ERROR
{
	RTREEMBR error_rect;
	struct DRC_ERROR *next = NULL;
} DRC_ERROR;
*/

typedef struct netlist
{
	double num;
	double xmin;
	double ymin;
	double xmax;
	double ymax;
	int net;
	int layer_num;
	char *metal_type;
	struct netlist *next = NULL;
} net;

void printrule();
net* read(char  file_name[]);
void critical_to_matrix(int critical_type[], critical_net *critical_net_head);
void layer_dummy_insert(RTREENODE *root, FILE *fPtr, int layer, RTREENODE *root_critical_expand);
void printPrev(FILE *fPtr);
void printEnd(FILE *fPtr);
REALTYPE dummymetalinsert(RTREENODE **node, RTREEMBR *window, double fill_width, double space, FILE *fPtr, int layer, int mode, RTREENODE **root_critical_expand);
void pattern(RTREENODE **node, RTREEMBR *window, double space, FILE *fPtr, int layer);
int horizontal_vertical(RTREEMBR *window, RTREENODE *root);
void insert_hori_rect_dummy(RTREENODE *root, RTREEMBR *window, double space, double fill_width, int layer, FILE *fPtr);
void insert_vert_rect_dummy(RTREENODE *root, RTREEMBR *window, double space, double fill_width, int layer, FILE *fPtr);
template <typename T>
void check_layer(RTREENODE *root, int layer, FILE *fPtr, stack<T>& stk, RTREENODE *root_critical_expand);
void lastcheck(RTREENODE *root, int layer);
void print_rect(RTREEMBR *rect, int layer, FILE *fPtr);
void insert_empty_window(RTREENODE **root, RTREEMBR *window, int layer, FILE *fPtr, double width);

void printrule()
{
	printf("window width: %3.f\n", window_width);
	for(int i=0; i<total_layer; i++)
	{
		printf("%d %5.f %5.f %5.f %5.f %5.f\n", i, min_width[i], min_space[i], max_fill_width[i], min_density[i], max_density[i]);
	}
}

net* read(char  file_name[])
{
	char buffer[256];
	const char *delim = " ";
	net *front_net = NULL;
	FILE *fPtr = fopen(file_name, "r");     /* open file pointer */
	if(fPtr) // if file exist...
	{
		fgets(buffer, 256, fPtr);
#ifdef design_transform
		chip_boundary.bound[1] = atof(strtok(buffer, delim));
		chip_boundary.bound[0] = atof(strtok(NULL, delim));
		chip_boundary.bound[3] = atof(strtok(NULL, delim));
		chip_boundary.bound[2] = atof(strtok(NULL, ":"));
#else
		chip_boundary.bound[0] = atof(strtok(buffer, delim));
		chip_boundary.bound[1] = atof(strtok(NULL, delim));
		chip_boundary.bound[2] = atof(strtok(NULL, delim));
		chip_boundary.bound[3] = atof(strtok(NULL, ";"));
#endif
		while(fgets(buffer, 256, fPtr)!=NULL)
		{
			net *tmp = new net;
			tmp->num = atof(strtok(buffer, delim));
			tmp->xmin = atof(strtok(NULL, delim));
			tmp->ymin = atof((strtok(NULL, delim)));
			tmp->xmax = atof((strtok(NULL, delim)));
			tmp->ymax = atof((strtok(NULL, delim)));
			tmp->net = atoi((strtok(NULL, delim)));
			tmp->layer_num = atoi((strtok(NULL, delim)));
			char *temp = strtok(NULL, "\n");
			tmp->metal_type = new char[strlen(temp) + 1];
			strcpy(tmp -> metal_type, temp);
			net *rear;
			if(front_net == NULL)
			{
				front_net = tmp;
				rear = tmp;
			}
			else
			{
				rear->next = tmp;
				rear = tmp;
			}
		}
	}
	fclose(fPtr);
	return front_net;
}

void critical_to_matrix(int critical_type[], critical_net *critical_net_head)
{
	critical_net *ptr = critical_net_head;
	while(ptr != NULL)
	{
		critical_type[ptr->id] = 1;
		critical_net *prev = ptr;
		ptr = ptr->next;
		delete prev;
	}
}

void layer_dummy_insert(RTREENODE *root, FILE *fPtr, int layer, RTREENODE *root_critical_expand)
{
    /*
	DRC_ERROR
        *head = NULL,
        *tail = NULL;
    */
    stack<DRC_ERRMBR> stk;
	RTREEMBR window_rect = {
	    chip_boundary.bound[0],
	    chip_boundary.bound[1],
	    chip_boundary.bound[0] + window_width / 2.,
	    chip_boundary.bound[1] + window_width / 2.
    };

	while(window_rect.bound[3] <= chip_boundary.bound[3])
	{
		while(window_rect.bound[2] <= chip_boundary.bound[2])
		{
#ifdef svg
			fprintf(fPtr, "<rect width=\"%f\" height=\"%f\" x=\"%f\" y=\"%f\" style=\"fill:none;stroke:#000000;stroke-width:10;stroke-miterlimit:4;stroke-dasharray:none;fill-opacity:1;opacity:1;stroke-opacity:1\"/>\n", window_rect.bound[2] - window_rect.bound[0], window_rect.bound[3] - window_rect.bound[1], window_rect.bound[0] - chip_boundary.bound[0], chip_boundary.bound[3] - window_rect.bound[3]);
#endif
			double fill_width = max_fill_width[layer];
			REALTYPE density;
			if((density = RTreeSearchDensity(root, &window_rect)) == 0)
				insert_empty_window(&root, &window_rect, layer, fPtr, window_width / 2.);
			else if(density < min_density[layer]) do{
                density = dummymetalinsert(&root, &window_rect, fill_width, min_space[layer], fPtr, layer, 0, &root_critical_expand);
                fill_width = ceil(fill_width * exp(alpha));
			} while(density < min_density[layer] && fill_width >= min_width[layer]);
			/*
            while((density = RTreeSearchDensity(root, &window_rect)) < min_density[layer] && fill_width >= min_width[layer]) {
                dummymetalinsert(&root, &window_rect, fill_width, min_space[layer], fPtr, layer, 0, &root_critical_expand);
                fill_width = ceil(fill_width * exp(alpha));
            }*/
			if(density < min_density[layer])
			{
				fill_width = min_width[layer];
				density = dummymetalinsert(&root, &window_rect, fill_width, min_space[layer], fPtr, layer, 0,  &root_critical_expand);
				if(density < min_density[layer])
				{
				    stk.push(window_rect);
				    /*
					DRC_ERROR *tmp = new DRC_ERROR;
					tmp->error_rect = window_rect;
					if(head == NULL)
					{
                        head = tmp;
                        tail = tmp;
					} else {
						tail->next = tmp;
						tail = tmp;
					}
					*/
#ifdef svg
					fprintf(fPtr, "<rect width=\"%f\" height=\"%f\" x=\"%f\" y=\"%f\" style=\"fill:none;stroke:#ff6600;stroke-width:100;stroke-miterlimit:4;stroke-dasharray:none;fill-opacity:1;opacity:1;stroke-opacity:1\"/>\n", window_rect.bound[2] - window_rect.bound[0], window_rect.bound[3] - window_rect.bound[1], window_rect.bound[0] - chip_boundary.bound[0], chip_boundary.bound[3] - window_rect.bound[3]);
#endif
				}
			}
			window_rect.bound[0] = window_rect.bound[0] + window_width / 2.;
			window_rect.bound[2] = window_rect.bound[0] + window_width / 2.;
		}
		window_rect.bound[1] = window_rect.bound[1] + window_width / 2.;
		window_rect.bound[3] = window_rect.bound[1] + window_width / 2.;
		window_rect.bound[0] = chip_boundary.bound[0];
		window_rect.bound[2] = window_rect.bound[0] + window_width / 2.;
	}
	if(stk.head != NULL)
	{
		printf("Checking\n");
		check_layer(root, layer, fPtr, stk, root_critical_expand);
	}
	else
		printf("Layer %d no need to check\n", layer+1);
	//lastcheck(root, layer);
}

void printPrev(FILE *fPtr)
{
	fprintf(fPtr, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?><svg xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:cc=\"http://creativecommons.org/ns#\" xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" xmlns:svg=\"http://www.w3.org/2000/svg\" xmlns=\"http://www.w3.org/2000/svg\" id=\"svg8\" version=\"1.1\"> <defs id=\"defs2\" /> <metadata id=\"metadata5\"> <rdf:RDF> <cc:Work rdf:about=\"\"> <dc:format>image/svg+xml</dc:format> <dc:type rdf:resource=\"http://purl.org/dc/dcmitype/StillImage\" /> <dc:title></dc:title> </cc:Work> </rdf:RDF> </metadata> <g id=\"layer1\">\n");
}

void printEnd(FILE *fPtr)
{
	fprintf(fPtr, "</g></svg>");
}

//REALTYPE RTreeInsertRect_density(RTREEMBR *rc, int tid, RTREENODE **root, int height)

REALTYPE dummymetalinsert(RTREENODE **node, RTREEMBR *window, double fill_width, double space, FILE *fPtr, int layer, int mode, RTREENODE **root_critical_expand)
{
	double x = window->bound[0] - space;
	double y = window->bound[1] - space;
	double move_space;
	if(mode == 0)
	{
		if(fill_width == min_width[layer])
			move_space = 5.;
		else
			move_space = 60.;
	}
	else//check
	{
		if(fill_width == min_width[layer])
			move_space = 1.;
		else
			move_space = 20.;
	}

	while((y + space < window->bound[3]) && (y + 2 * space + fill_width < chip_boundary.bound[3]))
	{
		while((x + space < window->bound[2]) && (x + 2 * space + fill_width < chip_boundary.bound[2]))
		{
			RTREEMBR outer_rect = {{x, y, x + 2 * space + fill_width, y + 2 * space + fill_width}};
			if(!RTreeLeafOverlap(*node, &outer_rect) && (!RTreeLeafOverlap(*root_critical_expand, &outer_rect) || mode))
			{
				RTREEMBR inner_rect = {{x + space, y + space, x + space + fill_width, y + space + fill_width}};
				if(fill_width == max_fill_width[layer])
					pattern(node, &inner_rect, space, fPtr, layer);
				else
				{
					RTreeInsertRect(&inner_rect, ++fill_id, node, 0);
#ifdef svg
					if(mode)
						fprintf(fPtr, "<rect width=\"%f\" height=\"%f\" x=\"%f\" y=\"%f\" style=\"fill:#d4aa00;stroke:none;stroke-width:0;stroke-miterlimit:4;stroke-dasharray:none;fill-opacity:1\"/>\n", inner_rect.bound[2] - inner_rect.bound[0], inner_rect.bound[3] - inner_rect.bound[1], inner_rect.bound[0] - chip_boundary.bound[0], chip_boundary.bound[3] - inner_rect.bound[3]);
					else
						fprintf(fPtr, "<rect width=\"%f\" height=\"%f\" x=\"%f\" y=\"%f\" style=\"fill:#cd8088;stroke:none;stroke-width:0;stroke-miterlimit:4;stroke-dasharray:none;fill-opacity:1\"/>\n", inner_rect.bound[2] - inner_rect.bound[0], inner_rect.bound[3] - inner_rect.bound[1], inner_rect.bound[0] - chip_boundary.bound[0], chip_boundary.bound[3] - inner_rect.bound[3]);
#else
					fprintf(fPtr, "%.f %.f %.f %.f %.f 0 %d fill\n", fill_id, inner_rect.bound[0], inner_rect.bound[1], inner_rect.bound[2], inner_rect.bound[3], layer + 1);
#endif
				}
				x = x + fill_width + space;
				REALTYPE density;
				if( (density = RTreeSearchDensity(*node, window)) > min_density[layer])
					return density;
			}
			else
				x = x + move_space;
		}
		y = y + move_space;
		x = window->bound[0] - space;
	}
}

void pattern(RTREENODE **node, RTREEMBR *window, double space, FILE *fPtr, int layer)
{
	double x = window->bound[0];
	double y = window->bound[1];
	double width = floor((max_fill_width[layer] - 1 * space) / 2);
	while(y + width <= window->bound[3])
	{
		while(x + width <= window->bound[2])
		{
			RTREEMBR rect = {{x, y, x + width, y + width}};

			RTreeInsertRect(&rect, ++fill_id, node, 0);
#ifdef svg
			fprintf(fPtr, "<rect width=\"%f\" height=\"%f\" x=\"%f\" y=\"%f\" style=\"fill:#000080;stroke:none;stroke-width:0;stroke-miterlimit:4;stroke-dasharray:none;fill-opacity:1\"/>\n", rect.bound[2] - rect.bound[0], rect.bound[3] - rect.bound[1], rect.bound[0] - chip_boundary.bound[0], chip_boundary.bound[3] - rect.bound[3]);
#else
			fprintf(fPtr, "%.f %.f %.f %.f %.f 0 %d fill\n", fill_id, rect.bound[0], rect.bound[1], rect.bound[2], rect.bound[3], layer + 1);
#endif
			x = x + width + space;
		}
		x = window->bound[0];
		y = y + width + space;
	}
}

//vertical design(1) or horizontal design(0)
int horizontal_vertical(RTREEMBR *window, RTREENODE *root)
{
	int horizontal_counter = 0;
	int vertical_counter = 0;
	RTREEMBR    horizontal_check_line = {{window->bound[0], window->bound[1], window->bound[2], window->bound[1]}};//add y(1, 3)
	RTREEMBR    vertical_check_line = {{window->bound[0], window->bound[1], window->bound[0], window->bound[3]}};//add x(0, 2)
	while((horizontal_check_line.bound[1] <= window->bound[3]) && (vertical_check_line.bound[0] <= window->bound[2]))
	{
		if(!RTreeLeafOverlap(root, &horizontal_check_line))
			horizontal_counter++;
		if(!RTreeLeafOverlap(root, &vertical_check_line))
			vertical_counter++;
		horizontal_check_line.bound[1] = horizontal_check_line.bound[1] + 10.;
		horizontal_check_line.bound[3] = horizontal_check_line.bound[1];
		vertical_check_line.bound[0] = vertical_check_line.bound[0] + 10.;
		vertical_check_line.bound[2] = vertical_check_line.bound[0];
	}
	if(horizontal_counter > vertical_counter)
		return 0;
	else
		return 1;
}

void insert_hori_rect_dummy(RTREENODE *root, RTREEMBR *window, double space, double fill_width, int layer, FILE *fPtr)
{
	double x = window->bound[0] - space;
	double y = window->bound[1] - space;
	double y_move_space = 1.;
	double x_move_space = 10.;
	double length = 0.;
	while(y + 2 * space + fill_width <= window->bound[3])
	{
		int insert_flag = 0;
		while(x + 2 * space + fill_width <= window->bound[2])
		{
			RTREEMBR outer_rect = {{x, y, x + 2 * space + fill_width, y + 2 * space + fill_width}};
			if(!RTreeLeafOverlap(root, &outer_rect))
			{
				int flag = 0;
				while(!flag)
				{
					RTREEMBR temp_rect = outer_rect;
					temp_rect.bound[2] = temp_rect.bound[2] + fill_width;
					length = temp_rect.bound[2] - temp_rect.bound[0] - 2 * space;
					if((RTreeLeafOverlap(root, &temp_rect)) || (length > max_fill_width[layer]) || (temp_rect.bound[2] - space > chip_boundary.bound[2]))
					{
						RTREEMBR inner_rect = {{outer_rect.bound[0] + space, outer_rect.bound[1] + space, outer_rect.bound[2] - space, outer_rect.bound[3] - space}};
						if((temp_rect.bound[2] - space > chip_boundary.bound[2]))
							inner_rect.bound[2] = chip_boundary.bound[2];
						RTreeInsertRect(&inner_rect, ++fill_id, &root, 0);
#ifdef svg
						fprintf(fPtr, "<rect width=\"%f\" height=\"%f\" x=\"%f\" y=\"%f\" style=\"fill:#d3bc5f;stroke:none;stroke-width:0;stroke-miterlimit:4;stroke-dasharray:none;fill-opacity:1\"/>\n", inner_rect.bound[2] - inner_rect.bound[0], inner_rect.bound[3] - inner_rect.bound[1], inner_rect.bound[0] - chip_boundary.bound[0], chip_boundary.bound[3] - inner_rect.bound[3]);
#else
						fprintf(fPtr, "%.f %.f %.f %.f %.f 0 %d fill\n", fill_id, inner_rect.bound[0], inner_rect.bound[1], inner_rect.bound[2], inner_rect.bound[3], layer + 1);
#endif
						x = inner_rect.bound[2];
						insert_flag = 1;
						flag = 1;
					}
					else
						outer_rect = temp_rect;
				}
			}
			else
				x = x + x_move_space;
		}
		if(insert_flag)
			y = y + 2 * space + 2 * fill_width;
		else
			y = y + y_move_space;
		x = window->bound[0] - space;
	}
}

void insert_vert_rect_dummy(RTREENODE *root, RTREEMBR *window, double space, double fill_width, int layer, FILE *fPtr)
{
	double x = window->bound[0] - space;
	double y = window->bound[1] - space;
	double x_move_space = 1.;
	double y_move_space = 10.;
	double length = 0.;
	while(x + 2 * space + fill_width <= window->bound[2])
	{
		int insert_flag = 0;
		while(y + 2 * space + fill_width <= window->bound[3])
		{
			RTREEMBR outer_rect = {{x, y, x + 2 * space + fill_width, y + 2 * space + fill_width}};
			if(!RTreeLeafOverlap(root, &outer_rect))
			{
				int flag = 0;
				while(!flag)
				{
					RTREEMBR temp_rect = outer_rect;
					temp_rect.bound[3] = temp_rect.bound[3] + fill_width;
					length = temp_rect.bound[3] - temp_rect.bound[1] - 2 * space;
					if((RTreeLeafOverlap(root, &temp_rect)) || (length > max_fill_width[layer]) || (temp_rect.bound[3] - space > chip_boundary.bound[3]))
					{
						RTREEMBR inner_rect = {{outer_rect.bound[0] + space, outer_rect.bound[1] + space, outer_rect.bound[2] - space, outer_rect.bound[3] - space}};
						if((temp_rect.bound[3] - space > chip_boundary.bound[3]))
							inner_rect.bound[3] = chip_boundary.bound[3];
						RTreeInsertRect(&inner_rect, ++fill_id, &root, 0);
#ifdef svg
						fprintf(fPtr, "<rect width=\"%f\" height=\"%f\" x=\"%f\" y=\"%f\" style=\"fill:#d3bc5f;stroke:none;stroke-width:0;stroke-miterlimit:4;stroke-dasharray:none;fill-opacity:1\"/>\n", inner_rect.bound[2] - inner_rect.bound[0], inner_rect.bound[3] - inner_rect.bound[1], inner_rect.bound[0] - chip_boundary.bound[0], chip_boundary.bound[3] - inner_rect.bound[3]);
#else
						fprintf(fPtr, "%.f %.f %.f %.f %.f 0 %d fill\n", fill_id, inner_rect.bound[0], inner_rect.bound[1], inner_rect.bound[2], inner_rect.bound[3], layer + 1);
#endif
						y = inner_rect.bound[3];
						insert_flag = 1;
						flag = 1;
					}
					else
						outer_rect = temp_rect;
				}
			}
			else
				y = y + y_move_space;
		}
		if(insert_flag)
			x = x + 2 * space + 2 * fill_width;
		else
			x = x + x_move_space;
		y = window->bound[1] - space;
	}
}
#ifndef svg
#define PRINT_DENSITY_INSUFFICIENT do {\
        printf("Trouble!!\nlayer %d %.f %.f %.f %.f density insufficient\nOriginal Density %f After density %f\n", layer+1, window_rect.bound[0], window_rect.bound[1], window_rect.bound[2], window_rect.bound[3], original_density, RTreeSearchDensity(root, &window_rect));\
} while (false)
#else
#define PRINT_DENSITY_INSUFFICIENT do {\
        printf("Trouble!!\nlayer %d %.f %.f %.f %.f density insufficient\nOriginal Density %f After density %f\n", layer+1, window_rect.bound[0], window_rect.bound[1], window_rect.bound[2], window_rect.bound[3], original_density, RTreeSearchDensity(root, &window_rect));\
       fprintf(fPtr, "<rect width=\"%f\" height=\"%f\" x=\"%f\" y=\"%f\" style=\"fill:none;stroke:#fd0000;stroke-width:100;stroke-miterlimit:4;stroke-dasharray:none;fill-opacity:1;opacity:1;stroke-opacity:1\"/>\n", window_rect.bound[2] - window_rect.bound[0], window_rect.bound[3] - window_rect.bound[1], window_rect.bound[0] - chip_boundary.bound[0], chip_boundary.bound[3] - window_rect.bound[3]);\
} while (false)
#endif

template <typename T>
void check_layer(RTREENODE *root, int layer, FILE *fPtr, stack<T>& stk, RTREENODE *root_critical_expand)
{
	int way;
	//DRC_ERROR* prev;
	frame<T> *head;
	while((head = stk.pop()) != NULL)
	{
		RTREEMBR window = head->data;
		RTREEMBR left_up = {{window.bound[0] - window_width/2., window.bound[1], window.bound[2], window.bound[3] + window_width / 2.}};
		RTREEMBR left_down = {{window.bound[0] - window_width/2., window.bound[1] - window_width/2., window.bound[2], window.bound[3]}};
		RTREEMBR right_up = {{window.bound[0], window.bound[1], window.bound[2]  + window_width / 2., window.bound[3]  + window_width / 2.}};
		RTREEMBR right_down = {{window.bound[0], window.bound[1] - window_width/2., window.bound[2]  + window_width / 2., window.bound[3]}};
		for(int i = 0; i < 4; i++)
		{
			RTREEMBR window_rect;
			switch(i)
			{
			case 0 :
				window_rect = left_down;
				break;
			case 1 :
				window_rect = right_down;
				break;
			case 2 :
				window_rect = left_up;
				break;
			case 3 :
				window_rect = right_up;
				break;
			}
			way = horizontal_vertical(&window_rect, root);
			if(window_rect.bound[0] >= chip_boundary.bound[0] && window_rect.bound[1] >= chip_boundary.bound[1] && window_rect.bound[2] <= chip_boundary.bound[2] && window_rect.bound[3] <= chip_boundary.bound[3])
			{
				double original_density = RTreeSearchDensity(root, &window_rect);
				REALTYPE density = original_density;
				double fill_width = max_fill_width[layer];
                do {
					density = dummymetalinsert(&root, &window_rect, fill_width, min_space[layer], fPtr, layer, 1, &root_critical_expand);
					fill_width = ceil(fill_width * exp(alpha));
				} while( density < min_density[layer] && fill_width > min_width[layer]);
				if(density < min_density[layer])
				{
					//printf("Inserting rectangle dummy metal\n");
					fill_width = min_width[layer];
					if(!way)
					{
						insert_hori_rect_dummy(root, &window_rect, min_space[layer], fill_width, layer, fPtr);
						if(RTreeSearchDensity(root, &window_rect) < min_density[layer]){
							insert_vert_rect_dummy(root, &window_rect, min_space[layer], fill_width, layer, fPtr);
                            if(RTreeSearchDensity(root, &window_rect) < min_density[layer]) PRINT_DENSITY_INSUFFICIENT;
						}
					}
					else
					{
						insert_vert_rect_dummy(root, &window_rect, min_space[layer], fill_width, layer, fPtr);
						if(RTreeSearchDensity(root, &window_rect) < min_density[layer]){
							insert_hori_rect_dummy(root, &window_rect, min_space[layer], fill_width, layer, fPtr);
                            if(RTreeSearchDensity(root, &window_rect) < min_density[layer]) PRINT_DENSITY_INSUFFICIENT;
						}
					}
					//printf("Finished inserting\n");
				}
			}
		}
		/*
		prev = head;
		head = head -> next;
		delete prev;
		*/
		delete head;
	}
	printf("Layer %d checked finished\n", layer + 1);
}

void lastcheck(RTREENODE *root, int layer)
{
	printf("Final check\n");
	double x = chip_boundary.bound[0];
	double y = chip_boundary.bound[1];
	while(x + window_width <= chip_boundary.bound[2])
	{
		while(y + window_width <= chip_boundary.bound[3])
		{
			RTREEMBR  window_rect = {{x, y, x + window_width, y + window_width}};
			if(RTreeSearchDensity(root, &window_rect) < min_density[layer])
				printf("Bad rect %f %f %f %f --- %f \n", window_rect.bound[0], window_rect.bound[1], window_rect.bound[2], window_rect.bound[3], RTreeSearchDensity(root, &window_rect));
			y = y + window_width / 2.;
		}
		x = x + window_width / 2.;
		y = chip_boundary.bound[1];
	}
}

void print_rect(RTREEMBR *rect, int layer, FILE *fPtr)
{
#ifdef svg
	fprintf(fPtr, "<rect width=\"%f\" height=\"%f\" x=\"%f\" y=\"%f\" style=\"fill:#c87137;stroke:none;stroke-width:0;stroke-miterlimit:4;stroke-dasharray:none;fill-opacity:1\"/>\n", rect->bound[2] - rect->bound[0], rect->bound[3] - rect->bound[1], rect->bound[0] - chip_boundary.bound[0], chip_boundary.bound[3] - rect->bound[3]);
#else
	fprintf(fPtr, "%.f %.f %.f %.f %.f 0 %d fill\n", fill_id, rect->bound[0], rect->bound[1], rect->bound[2], rect->bound[3], layer + 1);
#endif
}

void insert_empty_window(RTREENODE **root, RTREEMBR *window, int layer, FILE *fPtr, double width)
{
	double half_width = ceil(sqrt(width * width * min_density[layer]/4.)/2.);
	if(2 * half_width > max_fill_width[layer]) {
		RTREEMBR left_down_window = {{window->bound[0], window->bound[1], window->bound[0] + width/2., window->bound[1] + width/2.}};
		RTREEMBR left_up_window = {{window->bound[0], window->bound[1] + width/2., window->bound[0] + width/ 2., window->bound[3]}};
		RTREEMBR right_down_window = {{window->bound[0] + width/2., window->bound[1], window->bound[2], window->bound[1] + width/2.}};
		RTREEMBR right_up_window = {{window->bound[0] + width/2., window->bound[1] + width/2., window->bound[2], window->bound[3]}};
		insert_empty_window(root, &left_down_window, layer, fPtr, width/2.);
		insert_empty_window(root, &left_up_window, layer, fPtr, width/2.);
		insert_empty_window(root, &right_down_window, layer, fPtr, width/2.);
		insert_empty_window(root, &right_up_window, layer, fPtr, width/2.);
	} else {
		double x1 = window->bound[0] + width / 4.;
		double y1 = window->bound[1] + width / 4.;
		double x2 = window->bound[2] - width / 4.;
		double y2 = window->bound[3] - width / 4.;
		RTREEMBR left_down = {{x1 - half_width, y1 - half_width, x1 + half_width, y1 + half_width}};
		RTREEMBR left_up = {{x1 - half_width, y2 - half_width, x1 + half_width, y2 + half_width}};
		RTREEMBR right_down = {{x2 - half_width, y1 - half_width, x2 + half_width, y1 + half_width}};
		RTREEMBR right_up = {{x2 - half_width, y2 - half_width, x2 + half_width, y2 + half_width}};
		RTreeInsertRect(&left_down, ++fill_id, root, 0);
		print_rect(&left_down, layer, fPtr);
		RTreeInsertRect(&left_up, ++fill_id, root, 0);
		print_rect(&left_up, layer, fPtr);
		RTreeInsertRect(&right_down, ++fill_id, root, 0);
		print_rect(&right_down, layer, fPtr);
		RTreeInsertRect(&right_up, ++fill_id, root, 0);
		print_rect(&right_up, layer, fPtr);
		if(RTreeSearchDensity(*root, window) < min_density[layer])
			printf("**************failed**************\n");
	}
}


void rtree(char input_file_name[], char output_file_name[], critical_net *critical_net_head)
{
	time_t start=time(NULL);
	int max_critical_net = critical_net_head->id;
	int critical_type[max_critical_net + 1] = {};

	critical_to_matrix(critical_type, critical_net_head);

	printf("Input filename : %s\n", input_file_name);
	printrule();
#ifdef svg
	printf("Warning!!! SVG mode");
	for(int i=0; i<50; i++)
		printf("!");
	printf("\n");
	strcat(output_file_name, ".svg");
#endif

	RTREENODE* root[total_layer];
	RTREENODE* root_critical_expand[total_layer];

	for(int i=0; i<total_layer; i++)
	{
		root[i] = RTreeCreate();
		root_critical_expand[i] = RTreeCreate();
	}

	net *point = read(input_file_name);

	printf("Finished reading\n");
	FILE *fPtr = fopen(output_file_name,"w");

#ifdef svg
	printPrev(fPtr);
#else
	fprintf(fPtr, "%.f %.f %.f %.f; chip boundary\n", chip_boundary.bound[0], chip_boundary.bound[1], chip_boundary.bound[2], chip_boundary.bound[3]);
#endif

	while(point!=NULL)
	{
#ifdef design_transform
		RTREEMBR test_rects = {{point->ymin, point->xmin, point->ymax, point->xmax}};
#else
		RTREEMBR test_rects = {
		    point->xmin,
		    point->ymin,
		    point->xmax,
		    point->ymax
        };
#endif

		RTreeInsertRect(&test_rects, point->num, &root[point->layer_num - 1], 0);
		fill_id++;

		if(point->net <= max_critical_net && critical_type[point->net])
		{
			RTREEMBR critical_expand_rects = {
                point->xmin - critical_expand_factor * min_space[point->layer_num - 1],
                point->ymin - critical_expand_factor * min_space[point->layer_num - 1],
                point->xmax + critical_expand_factor * min_space[point->layer_num - 1],
                point->ymax + critical_expand_factor * min_space[point->layer_num - 1]
            };
			RTreeInsertRect(&critical_expand_rects, point->num, &root_critical_expand[point->layer_num - 1], 0);
#ifdef svg
			if(point->layer_num == svglayer)
				fprintf(fPtr, "<rect width=\"%f\" height=\"%f\" x=\"%f\" y=\"%f\" style=\"fill:none;stroke:#ff00ff;stroke-width:50;stroke-miterlimit:4;stroke-dasharray:none;fill-opacity:1;opacity:1;stroke-opacity:1\"/>\n", critical_expand_rects.bound[2] - critical_expand_rects.bound[0], critical_expand_rects.bound[3] - critical_expand_rects.bound[1], critical_expand_rects.bound[0] - chip_boundary.bound[0], chip_boundary.bound[3] - critical_expand_rects.bound[3]);
#endif
		}

#ifdef svg
		if(point->layer_num == svglayer)
		{
			if(point->net < max_critical_net && critical_type[point->net])
				fprintf(fPtr, "<rect width=\"%f\" height=\"%f\" x=\"%f\" y=\"%f\" style=\"fill:#ff00ff;stroke:none;stroke-width:0;stroke-miterlimit:4;stroke-dasharray:none;fill-opacity:1\"/>\n", test_rects.bound[2] - test_rects.bound[0], test_rects.bound[3] - test_rects.bound[1], test_rects.bound[0] - chip_boundary.bound[0], chip_boundary.bound[3] - test_rects.bound[3]);
			else
				fprintf(fPtr, "<rect width=\"%f\" height=\"%f\" x=\"%f\" y=\"%f\" style=\"fill:#000000;stroke:none;stroke-width:0;stroke-miterlimit:4;stroke-dasharray:none;fill-opacity:1\"/>\n", test_rects.bound[2] - test_rects.bound[0], test_rects.bound[3] - test_rects.bound[1], test_rects.bound[0] - chip_boundary.bound[0], chip_boundary.bound[3] - test_rects.bound[3]);
		}
#else
		//fprintf(fPtr, "%.f %.f %.f %.f %.f %d %d %s\n", fill_id, test_rects.bound[0], test_rects.bound[1], test_rects.bound[2], test_rects.bound[3], point->net, point->layer_num, point->metal_type);
#endif

		point=point->next;
	}

	printf("Finished building rtree\n");

#ifdef svg
	layer_dummy_insert(root[svglayer-1], fPtr, svglayer-1, root_critical_expand[svglayer-1]);
	printf("Layer %d completed\n", svglayer);
#else
	for(int i=0; i<total_layer; i++)
	{
		layer_dummy_insert(root[i], fPtr, i, root_critical_expand[i]);
		printf("Layer %d completed\n", i + 1);
	}
#endif

#ifdef svg
	printEnd(fPtr);
#endif

	fclose(fPtr);
	for(int i=0; i<total_layer; i++)
	{
		RTreeDestroy (root[i]);
		RTreeDestroy (root_critical_expand[i]);
	}

	printf("Time = %.f\n", difftime(time(NULL), start));
	printf("Program completed!\n");
}
