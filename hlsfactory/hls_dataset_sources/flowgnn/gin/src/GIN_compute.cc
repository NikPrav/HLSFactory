#include "dcl.h"
#include "load_inputs.h"
#include "conv_layer.h"
#include "finalize.h"

extern "C" {
void GIN_compute_graphs(
    int num_graphs,
    int* nums_of_nodes,
    int* nums_of_edges,
    int* reload_weights,
    FM_TYPE out[][NUM_TASK],
    node_feature_t* node_feature_in,
    edge_t* edge_list_in,
    edge_attr_t* edge_attr_in,
    WT_TYPE node_embedding_weight_in[][ND_FEATURE_TOTAL][EMB_DIM],
    WT_TYPE edge_embedding_weight_in[][NUM_LAYERS][ED_FEATURE_PER_LAYER][EMB_DIM],
    WT_TYPE node_mlp_1_weights[][NUM_LAYERS][MLP_1_OUT][EMB_DIM],
    WT_TYPE node_mlp_1_bias[][NUM_LAYERS][MLP_1_OUT],
    WT_TYPE node_mlp_2_weights[][NUM_LAYERS][EMB_DIM][MLP_1_OUT],
    WT_TYPE node_mlp_2_bias[][NUM_LAYERS][EMB_DIM],
    WT_TYPE graph_pred_weights_in[][NUM_TASK][EMB_DIM],
    WT_TYPE graph_pred_bias_in[][NUM_TASK]
)
{
#pragma HLS INTERFACE s_axilite port=return

#pragma HLS INTERFACE m_axi depth=(1) port=nums_of_nodes offset=slave bundle=mem
#pragma HLS INTERFACE m_axi depth=(1) port=nums_of_edges offset=slave bundle=mem
#pragma HLS INTERFACE m_axi depth=(1) port=reload_weights offset=slave bundle=mem
#pragma HLS INTERFACE m_axi depth=(1) port=out offset=slave bundle=mem
#pragma HLS INTERFACE m_axi depth=(MAX_NODE) port=node_feature_in offset=slave bundle=mem
#pragma HLS INTERFACE m_axi depth=(MAX_EDGE) port=edge_list_in offset=slave bundle=mem
#pragma HLS INTERFACE m_axi depth=(MAX_EDGE) port=edge_attr_in offset=slave bundle=mem
#pragma HLS INTERFACE m_axi depth=(1) port=node_embedding_weight_in offset=slave bundle=mem
#pragma HLS INTERFACE m_axi depth=(1) port=edge_embedding_weight_in offset=slave bundle=mem
#pragma HLS INTERFACE m_axi depth=(1) port=node_mlp_1_weights offset=slave bundle=mem
#pragma HLS INTERFACE m_axi depth=(1) port=node_mlp_1_bias offset=slave bundle=mem
#pragma HLS INTERFACE m_axi depth=(1) port=node_mlp_2_weights offset=slave bundle=mem
#pragma HLS INTERFACE m_axi depth=(1) port=node_mlp_2_bias offset=slave bundle=mem
#pragma HLS INTERFACE m_axi depth=(1) port=graph_pred_weights_in offset=slave bundle=mem
#pragma HLS INTERFACE m_axi depth=(1) port=graph_pred_bias_in offset=slave bundle=mem

    for (int graph = 0, weights_ndx = -1, nodes_offset = 0, edges_offset = 0; graph < num_graphs; graph++)
    {
#pragma HLS LOOP_TRIPCOUNT min=ANALYSIS_NUM_GRAPHS max=ANALYSIS_NUM_GRAPHS avg=ANALYSIS_NUM_GRAPHS
        int num_of_nodes = nums_of_nodes[graph];
        int num_of_edges = nums_of_edges[graph];
        bool reload_weights_graph = reload_weights[graph];

        if (reload_weights_graph)
        {
            weights_ndx++;
            load_weights(
                node_mlp_1_weights[weights_ndx],
                node_mlp_1_bias[weights_ndx],
                node_mlp_2_weights[weights_ndx],
                node_mlp_2_bias[weights_ndx],
                edge_embedding_weight_in[weights_ndx],
                graph_pred_weights_in[weights_ndx],
                graph_pred_bias_in[weights_ndx]
            );
        }

        load_graph(
            &edge_list_in[edges_offset],
            &edge_attr_in[edges_offset],
            num_of_nodes,
            num_of_edges
        );

        for (int i = 0; i <= NUM_LAYERS; i++)
        {
            if (i % 2 == 0)
                compute_CONV_layer(
                    i,
                    messages_ping,
                    messages_pong,
                    &node_feature_in[nodes_offset],
                    node_embedding_weight_in[weights_ndx],
                    out[graph],
                    num_of_nodes
                );
            else
                compute_CONV_layer(
                    i,
                    messages_pong,
                    messages_ping,
                    &node_feature_in[nodes_offset],
                    node_embedding_weight_in[weights_ndx],
                    out[graph],
                    num_of_nodes
                );
        }

        nodes_offset += num_of_nodes;
        edges_offset += num_of_edges;
    }
}
}
