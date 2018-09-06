#pragma once

struct critical_net
{
	int  id;
	struct critical_net *next;
};
//critical_net *critical_net_head;

void rtree(char input_file_name[], char output_file_name[], critical_net *critical_net_head);
