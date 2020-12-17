/*******************************************************************************
* Copyright (C) 2018-2020 Intel Corporation
*
* SPDX-License-Identifier: MIT
* 
* Modified by : 2020 Getac Summer Intern Quentin
******************************************************************************/

#include <dirent.h>
#include <gio/gio.h>
#include <gst/gst.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "gst/videoanalytics/video_frame.h"
#include <algorithm>
#include <ctime>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>


#include "opencv2/core/core_c.h"
#include <fstream>
#include "nlohmann/json.hpp"
#include <glib-unix.h>

using namespace std;
using json = nlohmann::json;
#define UNUSED(x) (void)(x)
GstElement* pipelineConstruct(string& pipeline_str);
string pipelineParse(json& config);


GstElement* pipelineConstruct(string& pipeline_str){
    auto launch_str = g_strdup_printf("%s",pipeline_str.c_str());
    g_print("PIPELINE: %s \n", launch_str);
    GstElement* pipeline;
    pipeline = gst_parse_launch(launch_str, NULL);
    g_free(launch_str);
    return pipeline;
}

string pipelineParse(json& config){
    gint CamID = config.find("CamID") == config.end() ? 0 : config["CamID"].get<int>();
    json pp_cfg = json(config["pipeline config"]);
    string pipeline_str;
    pipeline_str.reserve(1000);
    for (const auto& e : config["pipeline"].items()){
        string element = e.value().get<std::string>();
        pipeline_str.append(element);
        pipeline_str.append(" ");
        if(pp_cfg.find(element) != pp_cfg.end()){
            for (auto& a : pp_cfg[element].items()){
                pipeline_str.append(a.key());
                pipeline_str.append("=");
                pipeline_str.append(a.value().get<std::string>());
                if(element.compare("filesink")==0 && a.key().compare("location")==0)
                    pipeline_str.append(to_string(CamID) + ".mp4");
                else if(element.compare("textoverlay")==0 && a.key().compare("text")==0)
                    pipeline_str.append(to_string(CamID));
                pipeline_str.append(" ");
            }
        }
        pipeline_str.append("! ");
    }
    pipeline_str.erase(pipeline_str.end()-2,pipeline_str.end());
    return pipeline_str;
}

int main(int argc, char *argv[]) {

    gchar* config_path;
    GOptionEntry options[] =
    {
        {"config", 'c', 0, G_OPTION_ARG_FILENAME, &config_path, "Configuration json file path", NULL},
        {NULL}
    };
    GOptionContext *context = g_option_context_new("DESCRIPTION");
    g_option_context_add_main_entries(context, options, NULL);
    GError *error = NULL;
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        g_printerr("option parsing failed: %s\n", error->message);
        return 1;
    }


    // C style reading
    FILE * cfg_file = fopen(config_path, "r");
    if (!cfg_file) {
        printf("Cannot read the configuration json file!\n");
        exit(1);
    }
    json config = json::parse(cfg_file);
    fclose (cfg_file);
    // C++ style reading
    /*
    ifstream i(config_path);
    if (!i) {
        printf("Cannot read the configuration json file!\n");
        exit(1);
    }
    json config = json::parse(i);
    i.close();
    */


    gst_init (NULL, NULL);
    // parse pipeline from json
    string pipeline_str = pipelineParse(config);
    GstElement *pipeline;
    pipeline = pipelineConstruct(pipeline_str);

    int ret_code = 0;
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    GstBus *bus = gst_element_get_bus(pipeline);
    GstMessage *msg = gst_bus_poll(bus, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS), -1);

    if (msg) {
        switch(GST_MESSAGE_TYPE(msg)){
            case GST_MESSAGE_ERROR:
            {
                GError *err = NULL;
                gchar *dbg_info = NULL;
                gst_message_parse_error(msg, &err, &dbg_info);
                g_printerr("ERROR from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
                g_printerr("Debugging info: %s\n", (dbg_info) ? dbg_info : "none");
                g_free(dbg_info);
                g_error_free(err);
                break;
            }
            case GST_MESSAGE_EOS:{
                g_print("EOS\n");
                //finish = true;
            }
            default:{
                break;
            }
        }
    }
    gst_element_set_state(pipeline, GST_STATE_NULL);
    if (msg)
        gst_message_unref(msg);
    gst_object_unref(bus);

    // Free resources
    gst_object_unref(pipeline);
    return ret_code;
}
