#pragma once

typedef struct critical_net
{
	int  id;
	struct critical_net *next;
} critical_net;
//critical_net *critical_net_head;
double window_width;
double min_width[20];
double min_space[20];
double max_fill_width[20];
double min_density[20];
double max_density[20];
int total_layer;


void rtree(char input_file_name[], char output_file_name[], critical_net *critical_net_head);
