{% extends 'base_template.c' %}
{% import 'template_utils.h' as template_utils %}

{% block loader %}
static int get_exts(EGLDisplay display, const char **extensions) {
    *extensions = eglQueryString(display, EGL_EXTENSIONS);

    return extensions != NULL;
}

static int has_ext(const char *extensions, const char *name) {
    const char *loc;
    const char *terminator;
    if(extensions == NULL) {
        return 0;
    }
    while(1) {
        loc = strstr(extensions, extensions);
        if(loc == NULL) {
            return 0;
        }
        terminator = loc + strlen(extensions);
        if((loc == extensions || *(loc - 1) == ' ') &&
            (*terminator == ' ' || *terminator == '\0')) {
            return 1;
        }
        extensions = terminator;
    }
}

static int find_extensions{{ feature_set.api|upper }}(EGLDisplay display) {
    const char *extensions;
    if (!get_exts(display, &extensions)) return 0;

    {% for extension in feature_set.extensions %}
    GLAD_{{ extension.name }} = has_ext(extensions, "{{ extension.name }}");
    {% endfor %}

    return 1;
}

static int find_core{{ feature_set.api|upper }}(EGLDisplay *display) {
    int major, minor;
    const char *version;

    if (*display == NULL) {
        *display = EGL_NO_DISPLAY; /* this is usually NULL, better safe than sorry */
    }
    if (*display == EGL_NO_DISPLAY) {
        *display = eglGetCurrentDisplay();
    }
    if (*display == EGL_NO_DISPLAY) {
        *display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
    if (*display == EGL_NO_DISPLAY) {
        return 0;
    }

    version = eglQueryString(*display, EGL_VERSION);

#ifdef _MSC_VER
    sscanf_s(version, "%d.%d", &major, &minor);
#else
    sscanf(version, "%d.%d", &major, &minor);
#endif

    {% for feature in feature_set.features %}
    GLAD_{{ feature.name }} = (major == {{ feature.version.major }} && minor >= {{ feature.version.minor }}) || major > {{ feature.version.major }};
    {% endfor %}

    return 1;
}

int gladLoad{{ feature_set.api|upper }}(EGLDisplay *display, GLADloadproc load, void* userptr) {
    eglGetDisplay = (PFNEGLGETDISPLAYPROC)load("eglGetDisplay", userptr);
    eglGetCurrentDisplay = (PFNEGLGETCURRENTDISPLAYPROC)load("eglGetCurrentDisplay", userptr);
    eglQueryString = (PFNEGLQUERYSTRINGPROC)load("eglQueryString", userptr);
    if (eglGetDisplay == NULL || eglGetCurrentDisplay == NULL || eglQueryString == NULL) return 0;

    if (!find_core{{ feature_set.api|upper }}(display)) return 0;
    {% for feature in feature_set.features %}
    load_{{ feature.name }}(load, userptr);
    {% endfor %}

    if (!find_extensions{{ feature_set.api|upper }}(*display)) return 0;
    {% for extension in feature_set.extensions %}
    load_{{ extension.name }}(load, userptr);
    {% endfor %}

    return 1;
}

static void* glad_get_proc_from_userptr(const char* name, void *userptr) {
    return ((void* (*)(const char *name))userptr)(name);
}

int gladLoad{{ feature_set.api|upper }}Simple(EGLDisplay *display, GLADsimpleloadproc load) {
    return gladLoad{{ feature_set.api|upper }}(display, glad_get_proc_from_userptr, &load);
}

{% endblock %}