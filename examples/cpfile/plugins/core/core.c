/*
 * Copyright 2007 Johannes Lehtinen
 * This file is free software; Johannes Lehtinen gives unlimited
 * permission to copy, distribute and modify it.
 */

#include <stdlib.h>
#include <stdarg.h>
#include <cpluff.h>
#include "core.h"


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

/** Type for plugin_data_t structure */
typedef struct plugin_data_t plugin_data_t;

/** Type for registered_classifier_t structure */
typedef struct registered_classifier_t registered_classifier_t;

/** Plug-in instance data */
struct plugin_data_t {
	
	/** The plug-in context */
	cp_context_t *ctx;
	
	/** Number of registered classifiers */
	int num_classifiers;
	
	/** An array of registered classifiers */
	registered_classifier_t *classifiers; 
};

/** Registered classifier information */
struct registered_classifier_t {
	
	/** The priority of the classifier */
	int priority;
	
	/** The classifier data */
	classifier_t *classifier;
};


/* ------------------------------------------------------------------------
 * Internal functions
 * ----------------------------------------------------------------------*/

/**
 * Creates a new plug-in instance.
 */
static void *create(cp_context_t *ctx) {
	plugin_data_t *data = malloc(sizeof(plugin_data_t));
	if (data != NULL) {
		data->ctx = ctx;
		data->num_classifiers = 0;
		data->classifiers = NULL;
	} else {
		cp_log(ctx, CP_LOG_ERROR,
			"Insufficient memory for plug-in data.");
	}
	return data;
}

/**
 * Initializes and starts the plug-in.
 */
static int start(void *d) {
	plugin_data_t *data = d;
	cp_extension_t **cl_exts;
	int num_cl_exts;
	cp_status_t status;
	int i;
	
	/* Obtain list of registered classifiers */
	cl_exts = cp_get_extensions_info(
		data->ctx,
		"org.c-pluff.examples.cpfile.core.classifiers",
		&status,
		&num_cl_exts
	);
	if (cl_exts == NULL) {
		
		/* An error occurred and framework logged it */
		return status;
	}
	
	/* Allocate memory for classifier information, if any */
	if (data->num_classifiers > 0) {
		data->classifiers = malloc(
			num_cl_exts * sizeof(registered_classifier_t)
		);
		if (data->classifiers == NULL) {
			
			/* Memory allocation failed */
			cp_log(data->ctx, CP_LOG_ERROR,
				"Insufficient memory for classifier list.");
			return CP_ERR_RESOURCE;
		}
	} 
	
	/* Resolve classifier functions. This will implicitly start
	 * plug-ins providing the classifiers. */
	for (i = 0; i < num_cl_exts; i++) {
		const char *str;
		int pri;
		classifier_t *cl;
		
		/* Get the classifier function priority */
		str = cp_lookup_cfg_value(
			cl_exts[i]->configuration, "@priority"
		);
		if (str == NULL) {
			
			/* Classifier is missing mandatory priority */
			cp_log(data->ctx, CP_LOG_ERROR,
				"Ignoring classifier without priority.");
			continue;
		}
		pri = atoi(str);
		
		/* Resolve classifier data pointer */
		str = cp_lookup_cfg_value(
			cl_exts[i]->configuration, "@classifier");
		if (str == NULL) {
			
			/* Classifier symbol name is missing */
			cp_log(data->ctx, CP_LOG_ERROR,
				"Ignoring classifier without symbol name.");
			continue;
		}
		cl = cp_resolve_symbol(
			data->ctx,
			cl_exts[i]->plugin->identifier,
			str,
			NULL
		);
		if (cl == NULL) {
			
			/* Could not resolve classifier symbol */
			cp_log(data->ctx, CP_LOG_ERROR,
				"Ignoring classifier which could not be resolved.");
			continue;
		}
		
		/* Add classifier to the list of registered classifiers */
		data->classifiers[data->num_classifiers].priority = pri;
		data->classifiers[data->num_classifiers].classifier = cl;
		data->num_classifiers++;
	}
	
	/* Release extension information */
	cp_release_info(data->ctx, cl_exts);
	
	/* Successfully started */
	return CP_OK;
}

/**
 * Stops the plug-in and releases runtime resources.
 */
static void stop(void *d) {
	plugin_data_t *data = d;
	int i;
	
	/* Release classifier data, if any */
	if (data->classifiers != NULL) {
		
		/* Release classifier pointers */
		for (i = 0; i < data->num_classifiers; i++) {
			cp_release_symbol(
				data->ctx, data->classifiers[i].classifier
			);
		}
		
		/* Free local data */
		free(data->classifiers);
		data->classifiers = NULL;
		data->num_classifiers = 0;
	}
}

/**
 * Destroys a plug-in instance.
 */
static void destroy(void *d) {
	free(d);
}


/* ------------------------------------------------------------------------
 * Exported runtime information
 * ----------------------------------------------------------------------*/

/**
 * Plug-in runtime information for the framework. The name of this symbol
 * is stored in the plug-in descriptor.
 */
CP_EXPORT cp_plugin_runtime_t cp_ex_cpfile_core_funcs = {
	create,
	start,
	stop,
	destroy
};