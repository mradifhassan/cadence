/*
 * Cadence — typing test with a modern mechanical keyboard (GTK4 + Cairo/Pango + SDL2 audio)
 *
 * Keys:  type to start · Backspace corrects · Tab = new text · Esc = restart same text
 */
#define GDK_DISABLE_DEPRECATION_WARNINGS
#include <gtk/gtk.h>
#include <pango/pangocairo.h>
#include <SDL2/SDL.h>
#include <math.h>
#include <string.h>

/* Repository opened by the little GitHub icon under the keyboard. */
#ifndef CADENCE_REPO_URL
#define CADENCE_REPO_URL "https://github.com/mradifhassan/cadence"
#endif

/* Footer line: [github icon] • Open source project under <license> • <year> */
#ifndef CADENCE_LICENSE
#define CADENCE_LICENSE "GPL-3.0"
#endif
#ifndef CADENCE_YEAR
#define CADENCE_YEAR "2026"
#endif

#ifndef CADENCE_DATADIR
#define CADENCE_DATADIR "/usr/local/share/cadence"
#endif

/* =====================================================================
   Word list
   ===================================================================== */
static const char *const WORDS[] = {
    "the","be","to","of","and","a","in","that","have","it","for","not","on","with","he",
    "as","you","do","at","this","but","his","by","from","they","we","say","her","she","or",
    "an","will","my","one","all","would","there","their","what","so","up","out","if","about",
    "who","get","which","go","me","when","make","can","like","time","no","just","him","know",
    "take","people","into","year","your","good","some","could","them","see","other","than",
    "then","now","look","only","come","its","over","think","also","back","after","use","two",
    "how","our","work","first","well","way","even","new","want","because","any","these","give",
    "day","most","us","great","between","need","large","often","hand","high","place","hold",
    "turn","never","last","world","keep","still","learn","should","never","where","much",
    "before","move","right","boy","old","too","same","tell","does","set","three","air","play",
    "small","end","put","home","read","port","spell","add","land","here","must","big","such",
    "follow","act","why","ask","men","change","went","light","kind","off","need","house",
    "picture","try","again","animal","point","mother","near","build","self","earth","father",
    "head","stand","own","page","should","country","found","answer","school","grow","study",
    "still","learn","plant","cover","food","sun","four","between","state","keep","eye","never",
    "last","let","thought","city","tree","cross","farm","hard","start","might","story","saw",
    "far","sea","draw","left","late","run","while","press","close","night","real","life",
    "few","north","open","seem","together","next","white","children","begin","got","walk",
    "example","ease","paper","group","always","music","those","both","mark","book","letter",
    "until","mile","river","car","feet","care","second","enough","plain","girl","usual",
    "young","ready","above","ever","red","list","though","feel","talk","bird","soon","body",
    "dog","family","direct","pose","leave","song","measure","door","product","black","short",
    "numeral","class","wind","question","happen","complete","ship","area","half","rock",
    "order","fire","south","problem","piece","told","knew","pass","since","top","whole",
    "king","space","heard","best","hour","better","true","during","hundred","five","remember"
};
#define N_WORDS (G_N_ELEMENTS(WORDS))

/* =====================================================================
   Keyboard model: 60 % ANSI layout, 15 units wide
   ===================================================================== */
typedef enum { K_ALPHA, K_MOD, K_ACCENT, K_SPACE } KeyKind;

typedef struct {
    const char *label;   /* char keys: unshifted glyph (letters upper‑case); mods: caption */
    const char *shift;   /* shifted symbol or NULL */
    double      w;       /* width in units */
    KeyKind     kind;
    guint       sp1, sp2;/* special keyvals (0 for character keys) */
} KeyDef;

#define CH(l,s)   { l, s, 1.0, K_ALPHA, 0, 0 }
#define LT(l)     { l, NULL, 1.0, K_ALPHA, 0, 0 }
#define MOD(l,w,a,b) { l, NULL, w, K_MOD, a, b }

static const KeyDef ROW0[] = {
    CH("`","~"),CH("1","!"),CH("2","@"),CH("3","#"),CH("4","$"),CH("5","%"),CH("6","^"),
    CH("7","&"),CH("8","*"),CH("9","("),CH("0",")"),CH("-","_"),CH("=","+"),
    MOD("Backspace",2.0,GDK_KEY_BackSpace,0)
};
static const KeyDef ROW1[] = {
    MOD("Tab",1.5,GDK_KEY_Tab,GDK_KEY_ISO_Left_Tab),
    LT("Q"),LT("W"),LT("E"),LT("R"),LT("T"),LT("Y"),LT("U"),LT("I"),LT("O"),LT("P"),
    CH("[","{"),CH("]","}"),
    { "\\","|",1.5,K_ALPHA,0,0 }
};
static const KeyDef ROW2[] = {
    MOD("Caps",1.75,GDK_KEY_Caps_Lock,0),
    LT("A"),LT("S"),LT("D"),LT("F"),LT("G"),LT("H"),LT("J"),LT("K"),LT("L"),
    CH(";",":"),CH("'","\""),
    { "Enter",NULL,2.25,K_ACCENT,GDK_KEY_Return,GDK_KEY_KP_Enter }
};
static const KeyDef ROW3[] = {
    MOD("Shift",2.25,GDK_KEY_Shift_L,0),
    LT("Z"),LT("X"),LT("C"),LT("V"),LT("B"),LT("N"),LT("M"),
    CH(",","<"),CH(".",">"),CH("/","?"),
    MOD("Shift",2.75,GDK_KEY_Shift_R,0)
};
static const KeyDef ROW4[] = {
    MOD("Ctrl",1.25,GDK_KEY_Control_L,0), MOD("Super",1.25,GDK_KEY_Super_L,0),
    MOD("Alt",1.25,GDK_KEY_Alt_L,0),
    { "",NULL,6.25,K_SPACE,0,0 },
    MOD("Alt",1.25,GDK_KEY_Alt_R,GDK_KEY_ISO_Level3_Shift), MOD("Super",1.25,GDK_KEY_Super_R,0),
    MOD("Menu",1.25,GDK_KEY_Menu,0), MOD("Ctrl",1.25,GDK_KEY_Control_R,0)
};

typedef struct {
    const KeyDef *def;
    gunichar base_ch, shift_ch;   /* 0 for non‑character keys */
    double x, y, w;               /* units */
    double press;                 /* 0..1 animated travel */
    double glow;                  /* 0..1 fading feedback flash */
    int    status;                /* 0 neutral, 1 correct, 2 wrong */
    gboolean down;
    gint64 hold_until;            /* µs: keep visually down at least until */
} Key;

#define MAX_KEYS 80
#define KBD_UNIT 60.0
#define KBD_PAD  22.0
#define KBD_W (15 * KBD_UNIT + 2 * KBD_PAD)
#define KBD_H (5  * KBD_UNIT + 2 * KBD_PAD)

/* =====================================================================
   Audio (SDL2): polyphonic sample player with pitch variation
   ===================================================================== */
#define MAX_VOICES 12
#define AUDIO_RATE 44100
typedef struct { double pos, step; float gain; gboolean active; } Voice;

/* =====================================================================
   App state
   ===================================================================== */
typedef enum { ST_IDLE, ST_RUNNING, ST_DONE } RunState;

static const int    MODE_SECONDS[] = { 0, 15, 30, 60 };   /* 0 = fixed word count */
static const char  *MODE_LABELS[]  = { "25 words", "15 s", "30 s", "60 s" };
#define N_MODES 4
#define WORDS_MODE_COUNT 25

typedef struct {
    GtkWidget *window, *text_area, *kbd_area;
    GtkWidget *lbl_wpm, *lbl_acc, *lbl_time, *lbl_msg;
    GtkWidget *mode_btn[N_MODES];
    GtkWidget *sound_btn;
    GtkWidget *gh_widget;
    gboolean gh_hover;

    Key   keys[MAX_KEYS];
    int   n_keys;

    int      mode;
    RunState state;
    GString *target, *typed;
    guint    correct_ks, total_ks;
    gint64   start_us, end_us;
    guint    timeout_id;
    int      hint_key, hint_shift;
    double   text_line_h;
    guint    tick_id;
    gint64   last_frame_us;

    /* audio */
    SDL_AudioDeviceID dev;
    gint16 *sample; gsize sample_len;
    Voice   voices[MAX_VOICES];
    int     next_voice;
    gboolean sound_on;
} App;

/* =====================================================================
   Helpers
   ===================================================================== */
static void rrect(cairo_t *cr, double x, double y, double w, double h, double r) {
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r,     r, -G_PI / 2, 0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0,         G_PI / 2);
    cairo_arc(cr, x + r,     y + h - r, r, G_PI / 2,  G_PI);
    cairo_arc(cr, x + r,     y + r,     r, G_PI,      1.5 * G_PI);
    cairo_close_path(cr);
}
static void rgb_hex(cairo_t *cr, guint32 c, double a) {
    cairo_set_source_rgba(cr, ((c >> 16) & 255) / 255.0, ((c >> 8) & 255) / 255.0, (c & 255) / 255.0, a);
}
static void stop_rgb(cairo_pattern_t *p, double off, guint32 c, double a) {
    cairo_pattern_add_color_stop_rgba(p, off, ((c >> 16) & 255) / 255.0,
                                      ((c >> 8) & 255) / 255.0, (c & 255) / 255.0, a);
}

/* =====================================================================
   Keyboard layout construction / lookup
   ===================================================================== */
static void add_row(App *a, const KeyDef *defs, int n, int row, double x0) {
    double x = x0;
    for (int i = 0; i < n && a->n_keys < MAX_KEYS; i++) {
        Key *k = &a->keys[a->n_keys++];
        memset(k, 0, sizeof *k);
        k->def = &defs[i];
        k->x = x; k->y = row; k->w = defs[i].w;
        if (defs[i].kind == K_SPACE) { k->base_ch = k->shift_ch = ' '; }
        else if (!defs[i].sp1) {
            char c = defs[i].label[0];
            if (g_ascii_isalpha(c)) { k->base_ch = g_ascii_tolower(c); k->shift_ch = g_ascii_toupper(c); }
            else { k->base_ch = c; k->shift_ch = defs[i].shift ? defs[i].shift[0] : c; }
        }
        x += defs[i].w;
    }
}
static void build_layout(App *a) {
    a->n_keys = 0;
    add_row(a, ROW0, G_N_ELEMENTS(ROW0), 0, 0);
    add_row(a, ROW1, G_N_ELEMENTS(ROW1), 1, 0);
    add_row(a, ROW2, G_N_ELEMENTS(ROW2), 2, 0);
    add_row(a, ROW3, G_N_ELEMENTS(ROW3), 3, 0);
    add_row(a, ROW4, G_N_ELEMENTS(ROW4), 4, 0);
}
/* Find key for a keyval. prefer_shift_side: if non-NULL receives 1 when the keyval needs Shift. */
static int find_key_by_keyval(App *a, guint keyval) {
    for (int i = 0; i < a->n_keys; i++) {
        const KeyDef *d = a->keys[i].def;
        if (d->sp1 && (keyval == d->sp1 || (d->sp2 && keyval == d->sp2))) return i;
    }
    gunichar u = gdk_keyval_to_unicode(keyval);
    if (!u) return -1;
    gunichar lu = g_unichar_tolower(u);
    for (int i = 0; i < a->n_keys; i++) {
        Key *k = &a->keys[i];
        if (!k->base_ch) continue;
        if (k->base_ch == u || k->shift_ch == u || k->base_ch == lu) return i;
    }
    return -1;
}
static int find_key_by_char(App *a, gunichar c, gboolean *needs_shift) {
    if (needs_shift) *needs_shift = FALSE;
    for (int i = 0; i < a->n_keys; i++) {
        Key *k = &a->keys[i];
        if (!k->base_ch) continue;
        if (k->base_ch == c) return i;
        if (k->shift_ch == c) { if (needs_shift) *needs_shift = TRUE; return i; }
    }
    return -1;
}
static int find_special(App *a, guint kv, double prefer_x_gt) {
    int found = -1;
    for (int i = 0; i < a->n_keys; i++) {
        const KeyDef *d = a->keys[i].def;
        if (d->sp1 == kv) {
            if (found < 0) found = i;
            if (a->keys[i].x > prefer_x_gt) found = i;
        }
    }
    return found;
}

/* =====================================================================
   Audio
   ===================================================================== */
static void audio_cb(void *ud, Uint8 *stream, int len) {
    App *a = ud;
    gint16 *out = (gint16 *)stream;
    int n = len / (int)sizeof(gint16);
    float mix[2048];
    for (int off = 0; off < n; off += 2048) {
        int m = MIN(2048, n - off);
        memset(mix, 0, sizeof(float) * m);
        for (int v = 0; v < MAX_VOICES; v++) {
            Voice *vc = &a->voices[v];
            if (!vc->active) continue;
            for (int i = 0; i < m; i++) {
                gsize idx = (gsize)vc->pos;
                if (idx + 1 >= a->sample_len) { vc->active = FALSE; break; }
                float fr = (float)(vc->pos - idx);
                mix[i] += (a->sample[idx] * (1.f - fr) + a->sample[idx + 1] * fr) * vc->gain;
                vc->pos += vc->step;
            }
        }
        for (int i = 0; i < m; i++) {
            float s = mix[i];
            out[off + i] = (gint16)(s > 32767.f ? 32767 : (s < -32768.f ? -32768 : s));
        }
    }
}

static gchar *find_asset(const char *name) {
    gchar *exe = g_file_read_link("/proc/self/exe", NULL);
    gchar *dir = exe ? g_path_get_dirname(exe) : NULL;
    gchar *cands[5] = {
        dir ? g_build_filename(dir, "assets", name, NULL) : NULL,
        dir ? g_build_filename(dir, "..", "assets", name, NULL) : NULL,
        g_build_filename(g_get_current_dir(), "assets", name, NULL),
        g_build_filename(CADENCE_DATADIR, name, NULL),
        NULL };
    gchar *res = NULL;
    for (int i = 0; cands[i]; i++)
        if (!res && g_file_test(cands[i], G_FILE_TEST_IS_REGULAR)) res = g_strdup(cands[i]);
    for (int i = 0; i < 5; i++) g_free(cands[i]);
    g_free(dir); g_free(exe);
    return res;
}

/* Synthesised "thock" used when assets/click.wav is missing or unusable */
static void synth_click(App *a) {
    gsize n = AUDIO_RATE * 0.06;
    a->sample = g_new0(gint16, n);
    a->sample_len = n;
    GRand *r = g_rand_new_with_seed(1234);
    for (gsize i = 0; i < n; i++) {
        double t = (double)i / AUDIO_RATE;
        double noise = g_rand_double_range(r, -1, 1) * exp(-t * 260);
        double thock = sin(2 * G_PI * 170 * t) * exp(-t * 85);
        double tick  = sin(2 * G_PI * 2600 * t) * exp(-t * 500);
        a->sample[i] = (gint16)(32767 * 0.55 * (0.45 * noise + 0.8 * thock + 0.25 * tick));
    }
    g_rand_free(r);
}

static gboolean load_wav(App *a, const char *path) {
    SDL_AudioSpec spec; Uint8 *buf = NULL; Uint32 len = 0;
    if (!SDL_LoadWAV(path, &spec, &buf, &len)) return FALSE;
    SDL_AudioCVT cvt;
    int r = SDL_BuildAudioCVT(&cvt, spec.format, spec.channels, spec.freq, AUDIO_S16SYS, 1, AUDIO_RATE);
    if (r < 0) { SDL_FreeWAV(buf); return FALSE; }
    gsize out_len;
    if (r == 0) {
        out_len = len;
        a->sample = g_malloc(len);
        memcpy(a->sample, buf, len);
    } else {
        cvt.len = (int)len;
        cvt.buf = SDL_malloc((size_t)len * cvt.len_mult);
        memcpy(cvt.buf, buf, len);
        SDL_ConvertAudio(&cvt);
        out_len = (gsize)cvt.len_cvt;
        a->sample = g_malloc(out_len);
        memcpy(a->sample, cvt.buf, out_len);
        SDL_free(cvt.buf);
    }
    SDL_FreeWAV(buf);
    a->sample_len = out_len / sizeof(gint16);
    if (a->sample_len > AUDIO_RATE) a->sample_len = AUDIO_RATE;   /* cap at 1 s */
    return a->sample_len > 16;
}

static void audio_init(App *a) {
    a->sound_on = TRUE;
    gchar *wav = find_asset("click.wav");
    if (!wav || !load_wav(a, wav)) {
        g_free(a->sample); a->sample = NULL;
        synth_click(a);
        if (wav) g_message("Could not use %s — using built-in click", wav);
    }
    g_free(wav);

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) { g_warning("SDL audio: %s", SDL_GetError()); return; }
    SDL_AudioSpec want; SDL_zero(want);
    want.freq = AUDIO_RATE; want.format = AUDIO_S16SYS; want.channels = 1;
    want.samples = 512; want.callback = audio_cb; want.userdata = a;
    a->dev = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);   /* SDL converts to `want` for us */
    if (!a->dev) { g_warning("SDL open device: %s", SDL_GetError()); return; }
    SDL_PauseAudioDevice(a->dev, 0);
}

static void play_sound(App *a, double pitch, float gain) {
    if (!a->dev || !a->sound_on) return;
    SDL_LockAudioDevice(a->dev);
    Voice *v = &a->voices[a->next_voice];
    a->next_voice = (a->next_voice + 1) % MAX_VOICES;
    v->pos = 0; v->step = pitch; v->gain = gain; v->active = TRUE;
    SDL_UnlockAudioDevice(a->dev);
}
static void play_key_sound(App *a, const Key *k) {
    double jitter = 0.94 + 0.12 * g_random_double();
    if (k && k->def->kind == K_SPACE)       play_sound(a, 0.80 * jitter, 1.0f);
    else if (k && k->def->kind != K_ALPHA)  play_sound(a, 0.88 * jitter, 0.9f);
    else                                    play_sound(a, jitter, 0.85f);
}

/* =====================================================================
   Test text
   ===================================================================== */
static void append_words(App *a, int n) {
    for (int i = 0; i < n; i++) {
        if (a->target->len) g_string_append_c(a->target, ' ');
        g_string_append(a->target, WORDS[g_random_int_range(0, N_WORDS)]);
    }
}
static void new_text(App *a) {
    g_string_truncate(a->target, 0);
    append_words(a, MODE_SECONDS[a->mode] ? 60 : WORDS_MODE_COUNT);
}

/* =====================================================================
   Stats / run control
   ===================================================================== */
static guint count_correct(App *a) {
    guint c = 0;
    for (gsize i = 0; i < a->typed->len && i < a->target->len; i++)
        if (a->typed->str[i] == a->target->str[i]) c++;
    return c;
}
static double elapsed_s(App *a) {
    if (a->state == ST_IDLE) return 0;
    gint64 end = (a->state == ST_DONE) ? a->end_us : g_get_monotonic_time();
    return MAX(0.0, (end - a->start_us) / 1e6);
}
static void update_stats(App *a) {
    double el = elapsed_s(a);
    double wpm = (el > 0.0) ? (count_correct(a) / 5.0) / (MAX(el, 1.0) / 60.0) : 0.0;
    double acc = a->total_ks ? 100.0 * a->correct_ks / a->total_ks : 100.0;
    gchar *s;
    s = g_strdup_printf("%.0f", wpm);           gtk_label_set_text(GTK_LABEL(a->lbl_wpm), s); g_free(s);
    s = g_strdup_printf("%.0f%%", acc);         gtk_label_set_text(GTK_LABEL(a->lbl_acc), s); g_free(s);
    int dur = MODE_SECONDS[a->mode];
    if (dur) s = g_strdup_printf("%ds", (int)ceil(MAX(0.0, dur - el)));
    else     s = g_strdup_printf("%ds", (int)el);
    gtk_label_set_text(GTK_LABEL(a->lbl_time), s); g_free(s);
}
static void update_hint(App *a) {
    a->hint_key = a->hint_shift = -1;
    if (a->state == ST_DONE || a->typed->len >= a->target->len) return;
    gboolean sh;
    char c = a->target->str[a->typed->len];
    a->hint_key = find_key_by_char(a, (guchar)c, &sh);
    if (sh && a->hint_key >= 0) {
        gboolean right = a->keys[a->hint_key].x < 7.5;   /* press Shift with the opposite hand */
        a->hint_shift = find_special(a, right ? GDK_KEY_Shift_R : GDK_KEY_Shift_L, -1);
    }
}
static void finish(App *a) {
    if (a->state != ST_RUNNING) return;
    a->end_us = g_get_monotonic_time();
    int dur = MODE_SECONDS[a->mode];
    if (dur && (a->end_us - a->start_us) > dur * 1000000LL) a->end_us = a->start_us + dur * 1000000LL;
    a->state = ST_DONE;
    if (a->timeout_id) { g_source_remove(a->timeout_id); a->timeout_id = 0; }
    update_stats(a);
    double el = elapsed_s(a);
    double wpm = (count_correct(a) / 5.0) / (MAX(el, 1.0) / 60.0);
    double acc = a->total_ks ? 100.0 * a->correct_ks / a->total_ks : 100.0;
    gchar *m = g_strdup_printf("Finished  ·  %.0f WPM  ·  %.1f%% accuracy  ·  Tab for new text", wpm, acc);
    gtk_label_set_text(GTK_LABEL(a->lbl_msg), m); g_free(m);
    update_hint(a);
    gtk_widget_queue_draw(a->text_area);
    gtk_widget_queue_draw(a->kbd_area);
}
static gboolean on_timeout(gpointer ud) {
    App *a = ud;
    if (a->state != ST_RUNNING) { a->timeout_id = 0; return G_SOURCE_REMOVE; }
    int dur = MODE_SECONDS[a->mode];
    if (dur && elapsed_s(a) >= dur) { a->timeout_id = 0; finish(a); return G_SOURCE_REMOVE; }
    update_stats(a);
    return G_SOURCE_CONTINUE;
}
static void restart(App *a, gboolean fresh_text) {
    if (a->timeout_id) { g_source_remove(a->timeout_id); a->timeout_id = 0; }
    a->state = ST_IDLE;
    g_string_truncate(a->typed, 0);
    a->correct_ks = a->total_ks = 0;
    if (fresh_text || a->target->len == 0) new_text(a);
    gtk_label_set_text(GTK_LABEL(a->lbl_msg), "Start typing to begin  ·  Tab: new text  ·  Esc: restart");
    update_stats(a);
    update_hint(a);
    gtk_widget_queue_draw(a->text_area);
    gtk_widget_queue_draw(a->kbd_area);
}

/* =====================================================================
   Input handling
   ===================================================================== */
static int type_char(App *a, char c) {          /* returns 1 correct, 2 wrong, 0 ignored */
    if (a->state == ST_DONE || a->typed->len >= a->target->len) return 0;
    if (a->state == ST_IDLE) {
        a->state = ST_RUNNING;
        a->start_us = g_get_monotonic_time();
        a->timeout_id = g_timeout_add(100, on_timeout, a);
        gtk_label_set_text(GTK_LABEL(a->lbl_msg), " ");
    }
    gboolean ok = (a->target->str[a->typed->len] == c);
    g_string_append_c(a->typed, c);
    a->total_ks++; if (ok) a->correct_ks++;

    if (MODE_SECONDS[a->mode] && a->target->len - a->typed->len < 60) append_words(a, 30);
    if (!MODE_SECONDS[a->mode] && a->typed->len >= a->target->len) finish(a);
    update_hint(a);
    update_stats(a);
    gtk_widget_queue_draw(a->text_area);
    return ok ? 1 : 2;
}

static void press_key(App *a, int ki, int status) {
    if (ki < 0) return;
    Key *k = &a->keys[ki];
    gint64 now = g_get_monotonic_time();
    k->down = TRUE;
    k->hold_until = now + 70000;       /* minimum visible travel 70 ms */
    k->status = status;
    k->glow = 1.0;
    gtk_widget_queue_draw(a->kbd_area);
}

static gboolean on_key_pressed(GtkEventControllerKey *c, guint keyval, guint keycode,
                               GdkModifierType mods, gpointer ud) {
    (void)c; (void)keycode;
    App *a = ud;
    int ki = find_key_by_keyval(a, keyval);
    int status = 0;
    gboolean handled = FALSE;

    if (keyval == GDK_KEY_Escape) { restart(a, FALSE); return TRUE; }
    if (keyval == GDK_KEY_Tab || keyval == GDK_KEY_ISO_Left_Tab) {
        press_key(a, ki, 0); play_key_sound(a, ki >= 0 ? &a->keys[ki] : NULL);
        restart(a, TRUE);
        return TRUE;
    }

    if (mods & (GDK_CONTROL_MASK | GDK_ALT_MASK | GDK_SUPER_MASK)) {
        press_key(a, ki, 0);
        return FALSE;                                   /* let system shortcuts through */
    }

    if (keyval == GDK_KEY_BackSpace) {
        if (a->state != ST_DONE && a->typed->len > 0) {
            g_string_truncate(a->typed, a->typed->len - 1);
            update_hint(a); update_stats(a);
            gtk_widget_queue_draw(a->text_area);
        }
        handled = TRUE;
    } else {
        gunichar u = gdk_keyval_to_unicode(keyval);
        if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) handled = TRUE;
        else if (u >= 32 && u < 127) { status = type_char(a, (char)u); handled = TRUE; }
    }
    if (ki >= 0) { press_key(a, ki, status); play_key_sound(a, &a->keys[ki]); }
    return handled;
}

static void on_key_released(GtkEventControllerKey *c, guint keyval, guint keycode,
                            GdkModifierType mods, gpointer ud) {
    (void)c; (void)keycode; (void)mods;
    App *a = ud;
    int ki = find_key_by_keyval(a, keyval);
    if (ki >= 0) a->keys[ki].down = FALSE;
    /* A Shift release can change the keyval of a still‑held key; make sure nothing sticks. */
    gtk_widget_queue_draw(a->kbd_area);
}

static void on_active_changed(GObject *w, GParamSpec *p, gpointer ud) {
    (void)p; App *a = ud;
    if (!gtk_window_is_active(GTK_WINDOW(w)))
        for (int i = 0; i < a->n_keys; i++) a->keys[i].down = FALSE;
}

/* =====================================================================
   Typing text area (Pango, scrolling 3‑line window)
   ===================================================================== */
#define TEXT_FONT "JetBrains Mono,Ubuntu Mono,DejaVu Sans Mono,Monospace 22"

static void set_attr(PangoAttrList *l, PangoAttribute *at, int i) {
    at->start_index = i; at->end_index = i + 1; pango_attr_list_insert(l, at);
}
static void text_draw(GtkDrawingArea *area, cairo_t *cr, int w, int h, gpointer ud) {
    (void)area;
    App *a = ud;
    double maxw = MIN((double)w, 1000.0), x0 = (w - maxw) / 2.0;

    PangoLayout *lay = pango_cairo_create_layout(cr);
    PangoFontDescription *fd = pango_font_description_from_string(TEXT_FONT);
    pango_layout_set_font_description(lay, fd);
    pango_font_description_free(fd);
    pango_layout_set_width(lay, (int)((maxw - 8) * PANGO_SCALE));
    pango_layout_set_wrap(lay, PANGO_WRAP_WORD_CHAR);
    pango_layout_set_line_spacing(lay, 1.35f);
    pango_layout_set_text(lay, a->target->str, (int)a->target->len);

    PangoAttrList *al = pango_attr_list_new();
    gsize cur = MIN(a->typed->len, a->target->len ? a->target->len - 1 : 0);
    for (gsize i = 0; i < a->target->len; i++) {
        char t = a->target->str[i];
        if (i < a->typed->len) {
            gboolean ok = a->typed->str[i] == t;
            if (ok) set_attr(al, pango_attr_foreground_new(0xcdcd, 0xd6d6, 0xf4f4), i);
            else {
                set_attr(al, pango_attr_foreground_new(0xf3f3, 0x8b8b, 0xa8a8), i);
                if (t == ' ') {
                    set_attr(al, pango_attr_background_new(0xf3f3, 0x8b8b, 0xa8a8), i);
                    set_attr(al, pango_attr_background_alpha_new(0x5000), i);
                }
            }
        } else {
            set_attr(al, pango_attr_foreground_new(0x6c6c, 0x7070, 0x8686), i);
        }
        if (i == cur && a->state != ST_DONE) {
            set_attr(al, pango_attr_background_new(0xcbcb, 0xa6a6, 0xf7f7), i);
            set_attr(al, pango_attr_background_alpha_new(0x4000), i);
            set_attr(al, pango_attr_underline_new(PANGO_UNDERLINE_SINGLE), i);
        }
    }
    pango_layout_set_attributes(lay, al);
    pango_attr_list_unref(al);

    /* scroll so the cursor sits on the 2nd visible line */
    int cur_line = 0, dummy;
    pango_layout_index_to_line_x(lay, (int)cur, FALSE, &cur_line, &dummy);
    double scroll = 0, prev_y = 0, line_y = 0;
    PangoLayoutIter *it = pango_layout_get_iter(lay);
    int ln = 0;
    do {
        int y0, y1; pango_layout_iter_get_line_yrange(it, &y0, &y1);
        if (ln == cur_line) { line_y = y0; break; }
        prev_y = y0; ln++;
    } while (pango_layout_iter_next_line(it));
    pango_layout_iter_free(it);
    if (cur_line >= 1) scroll = prev_y / PANGO_SCALE;
    (void)line_y;

    cairo_save(cr);
    cairo_rectangle(cr, 0, 0, w, h); cairo_clip(cr);
    cairo_move_to(cr, x0 + 4, 4 - scroll);
    pango_cairo_show_layout(cr, lay);
    cairo_restore(cr);

    /* fade out bottom edge */
    cairo_pattern_t *p = cairo_pattern_create_linear(0, h - 30, 0, h);
    stop_rgb(p, 0, 0x11111b, 0); stop_rgb(p, 1, 0x11111b, 1);
    cairo_set_source(cr, p); cairo_rectangle(cr, 0, h - 30, w, 30); cairo_fill(cr);
    cairo_pattern_destroy(p);
    g_object_unref(lay);
}

/* =====================================================================
   Keyboard drawing
   ===================================================================== */
typedef struct { guint32 top1, top2, skirt1, skirt2, legend, legend_dim; } Palette;
static const Palette PAL_ALPHA  = { 0x4a4d63, 0x3b3d52, 0x2b2c3d, 0x1b1c29, 0xe6e9f7, 0x8b90ad };
static const Palette PAL_MOD    = { 0x34364a, 0x2b2d3f, 0x222334, 0x161722, 0xa6adc8, 0x7f849c };
static const Palette PAL_ACCENT = { 0xb4befe, 0x89b4fa, 0x5f7fd0, 0x3f5aa6, 0x181825, 0x181825 };

static void layout_text(cairo_t *cr, PangoLayout *lay, const char *txt, double size, gboolean bold,
                        double *tw, double *th) {
    PangoFontDescription *fd = pango_font_description_from_string(bold ? "Ubuntu,Inter,Sans Bold" : "Ubuntu,Inter,Sans");
    pango_font_description_set_absolute_size(fd, size * PANGO_SCALE);
    pango_layout_set_font_description(lay, fd);
    pango_font_description_free(fd);
    pango_layout_set_text(lay, txt, -1);
    PangoRectangle r; pango_layout_get_extents(lay, NULL, &r);
    *tw = (double)r.width / PANGO_SCALE; *th = (double)r.height / PANGO_SCALE;
    (void)cr;
}

static void draw_key(cairo_t *cr, PangoLayout *lay, App *a, int idx) {
    Key *k = &a->keys[idx];
    const KeyDef *d = k->def;
    const Palette *pal = d->kind == K_ACCENT ? &PAL_ACCENT : d->kind == K_MOD ? &PAL_MOD : &PAL_ALPHA;

    double gx = KBD_PAD + k->x * KBD_UNIT, gy = KBD_PAD + k->y * KBD_UNIT, gw = k->w * KBD_UNIT;
    double ox = gx + 2.5, oy = gy + 2.5, ow = gw - 5, oh = KBD_UNIT - 5;
    double p = k->press;
    double tx = ox + 5, ty = oy + 2.5 + 3.5 * p, tw = ow - 10, th = oh - 11.5;

    /* drop shadow on plate */
    rrect(cr, ox, oy + 3, ow, oh, 9); cairo_set_source_rgba(cr, 0, 0, 0, 0.45); cairo_fill(cr);

    /* skirt (cap sides) */
    cairo_pattern_t *sk = cairo_pattern_create_linear(0, oy, 0, oy + oh);
    stop_rgb(sk, 0, pal->skirt1, 1); stop_rgb(sk, 1, pal->skirt2, 1);
    rrect(cr, ox, oy, ow, oh, 9); cairo_set_source(cr, sk); cairo_fill(cr); cairo_pattern_destroy(sk);

    /* top face */
    cairo_pattern_t *tp = cairo_pattern_create_linear(0, ty, 0, ty + th);
    stop_rgb(tp, 0, pal->top1, 1); stop_rgb(tp, 1, pal->top2, 1);
    rrect(cr, tx, ty, tw, th, 7); cairo_set_source(cr, tp); cairo_fill(cr); cairo_pattern_destroy(tp);

    /* rim light */
    cairo_pattern_t *rim = cairo_pattern_create_linear(0, ty, 0, ty + th);
    cairo_pattern_add_color_stop_rgba(rim, 0, 1, 1, 1, 0.22);
    cairo_pattern_add_color_stop_rgba(rim, 0.35, 1, 1, 1, 0.03);
    cairo_pattern_add_color_stop_rgba(rim, 1, 0, 0, 0, 0.18);
    rrect(cr, tx + 0.5, ty + 0.5, tw - 1, th - 1, 6.5); cairo_set_source(cr, rim);
    cairo_set_line_width(cr, 1.0); cairo_stroke(cr); cairo_pattern_destroy(rim);

    /* hint outline (next key to press) */
    gboolean hinted = (idx == a->hint_key || idx == a->hint_shift);
    if (hinted && a->state != ST_DONE) {
        rrect(cr, tx, ty, tw, th, 7); rgb_hex(cr, 0xcba6f7, 0.16); cairo_fill(cr);
        rrect(cr, ox - 1, oy - 1, ow + 2, oh + 2, 10); rgb_hex(cr, 0xcba6f7, 0.95);
        cairo_set_line_width(cr, 2.0); cairo_stroke(cr);
    }

    /* press feedback glow */
    if (k->glow > 0.01) {
        guint32 col = k->status == 1 ? 0xa6e3a1 : k->status == 2 ? 0xf38ba8 : 0x89b4fa;
        rrect(cr, tx, ty, tw, th, 7); rgb_hex(cr, col, 0.55 * k->glow); cairo_fill(cr);
        rrect(cr, ox - 2, oy - 2, ow + 4, oh + 4, 11); rgb_hex(cr, col, 0.55 * k->glow);
        cairo_set_line_width(cr, 2.5); cairo_stroke(cr);
    }

    /* legends */
    double lw, lh;
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    if (d->kind == K_SPACE) return;
    if (d->sp1) {                                           /* modifier caption, left aligned */
        layout_text(cr, lay, d->label, 11.5, TRUE, &lw, &lh);
        rgb_hex(cr, pal->legend, 1);
        cairo_move_to(cr, tx + 10, ty + (th - lh) / 2 - 1);
        pango_cairo_show_layout(cr, lay);
    } else if (g_ascii_isalpha(d->label[0])) {              /* letter */
        layout_text(cr, lay, d->label, 20, TRUE, &lw, &lh);
        rgb_hex(cr, pal->legend, 1);
        cairo_move_to(cr, tx + (tw - lw) / 2, ty + (th - lh) / 2 - 1);
        pango_cairo_show_layout(cr, lay);
        if (k->base_ch == 'f' || k->base_ch == 'j') {       /* homing bump */
            rrect(cr, tx + tw / 2 - 7, ty + th - 8, 14, 2.5, 1.2);
            cairo_set_source_rgba(cr, 1, 1, 1, 0.35); cairo_fill(cr);
        }
    } else {                                                /* symbol: shifted on top, base below */
        layout_text(cr, lay, d->shift, 13, FALSE, &lw, &lh);
        rgb_hex(cr, pal->legend_dim, 1);
        cairo_move_to(cr, tx + (tw - lw) / 2, ty + 4);
        pango_cairo_show_layout(cr, lay);
        layout_text(cr, lay, d->label, 16, TRUE, &lw, &lh);
        rgb_hex(cr, pal->legend, 1);
        cairo_move_to(cr, tx + (tw - lw) / 2, ty + th - lh - 3);
        pango_cairo_show_layout(cr, lay);
    }
}

static void kbd_draw(GtkDrawingArea *area, cairo_t *cr, int w, int h, gpointer ud) {
    (void)area;
    App *a = ud;
    double s = MIN(w / KBD_W, h / KBD_H);
    cairo_translate(cr, (w - KBD_W * s) / 2, (h - KBD_H * s) / 2);
    cairo_scale(cr, s, s);

    /* case */
    rrect(cr, 3, 7, KBD_W - 6, KBD_H - 7, 20); cairo_set_source_rgba(cr, 0, 0, 0, 0.55); cairo_fill(cr);
    cairo_pattern_t *cs = cairo_pattern_create_linear(0, 0, 0, KBD_H);
    stop_rgb(cs, 0, 0x34364a, 1); stop_rgb(cs, 1, 0x161722, 1);
    rrect(cr, 0, 0, KBD_W, KBD_H - 3, 20); cairo_set_source(cr, cs); cairo_fill(cr); cairo_pattern_destroy(cs);
    rrect(cr, 0.5, 0.5, KBD_W - 1, KBD_H - 4, 19.5); cairo_set_source_rgba(cr, 1, 1, 1, 0.14);
    cairo_set_line_width(cr, 1); cairo_stroke(cr);

    /* plate */
    rrect(cr, 11, 11, KBD_W - 22, KBD_H - 25, 13); rgb_hex(cr, 0x0c0c14, 1); cairo_fill(cr);
    rrect(cr, 11.5, 11.5, KBD_W - 23, KBD_H - 26, 12.5); cairo_set_source_rgba(cr, 0, 0, 0, 0.6);
    cairo_set_line_width(cr, 1.5); cairo_stroke(cr);

    PangoLayout *lay = pango_cairo_create_layout(cr);
    for (int i = 0; i < a->n_keys; i++) draw_key(cr, lay, a, i);
    g_object_unref(lay);
}

/* Frame‑clock driven animation of key travel and glow decay */
static gboolean kbd_tick(GtkWidget *w, GdkFrameClock *fc, gpointer ud) {
    (void)w;
    App *a = ud;
    gint64 now = g_get_monotonic_time();
    gint64 ft = gdk_frame_clock_get_frame_time(fc);
    double dt = a->last_frame_us ? (ft - a->last_frame_us) / 1e6 : 0.016;
    a->last_frame_us = ft;
    dt = CLAMP(dt, 0.001, 0.05);
    gboolean dirty = FALSE;
    for (int i = 0; i < a->n_keys; i++) {
        Key *k = &a->keys[i];
        double target = (k->down || now < k->hold_until) ? 1.0 : 0.0;
        double rate = target > k->press ? 45.0 : 16.0;
        double np = k->press + (target - k->press) * MIN(1.0, rate * dt);
        if (fabs(np - target) < 0.01) np = target;
        double ng = k->glow > 0 ? MAX(0.0, k->glow - dt * (k->down ? 0.6 : 2.4)) : 0.0;
        if (np != k->press || ng != k->glow) dirty = TRUE;
        k->press = np; k->glow = ng;
    }
    if (dirty) gtk_widget_queue_draw(a->kbd_area);
    return G_SOURCE_CONTINUE;
}

/* =====================================================================
   UI
   ===================================================================== */
static const char *CSS =
"window { background: #11111b; color: #cdd6f4; }\n"
"headerbar { background: #181825; border-bottom: 1px solid #2a2b3c; box-shadow: none; color: #cdd6f4; min-height: 42px; }\n"
"headerbar button { background: transparent; background-image: none; color: #a6adc8; border: none; box-shadow: none; }\n"
"headerbar button:hover { background: #313244; color: #cdd6f4; }\n"
"headerbar button:checked { color: #cba6f7; }\n"
".app-title { font-weight: 800; letter-spacing: 4px; color: #cba6f7; }\n"
".mode-bar { background: #181825; border-radius: 12px; padding: 4px; }\n"
".mode-btn { background: transparent; background-image: none; color: #7f849c; border: none; box-shadow: none;\n"
"            border-radius: 9px; padding: 6px 18px; min-height: 0; font-weight: 600; }\n"
".mode-btn:hover { color: #cdd6f4; }\n"
".mode-btn:checked { background: #313244; color: #cba6f7; }\n"
".stat-value { font-size: 40px; font-weight: 700; color: #cdd6f4; font-family: 'JetBrains Mono','Ubuntu Mono',monospace; }\n"
".stat-value.accent { color: #cba6f7; }\n"
".stat-name { font-size: 11px; font-weight: 700; letter-spacing: 3px; color: #6c7086; }\n"
".hint { color: #7f849c; font-size: 13px; }\n"
".footer-text { color: #6c7086; font-size: 11px; }\n";


static GtkWidget *make_stat(const char *name, GtkWidget **value_out, gboolean accent) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *v = gtk_label_new("0");
    gtk_widget_add_css_class(v, "stat-value");
    if (accent) gtk_widget_add_css_class(v, "accent");
    gtk_label_set_width_chars(GTK_LABEL(v), 5);
    GtkWidget *n = gtk_label_new(name);
    gtk_widget_add_css_class(n, "stat-name");
    gtk_box_append(GTK_BOX(box), v);
    gtk_box_append(GTK_BOX(box), n);
    *value_out = v;
    return box;
}

static void on_mode_toggled(GtkToggleButton *b, gpointer ud) {
    App *a = ud;
    if (!gtk_toggle_button_get_active(b)) return;
    for (int i = 0; i < N_MODES; i++)
        if (GTK_WIDGET(b) == a->mode_btn[i]) { a->mode = i; break; }
    restart(a, TRUE);
}
static void on_sound_toggled(GtkToggleButton *b, gpointer ud) {
    App *a = ud;
    a->sound_on = gtk_toggle_button_get_active(b);
    gtk_button_set_icon_name(GTK_BUTTON(b), a->sound_on ? "audio-volume-high-symbolic" : "audio-volume-muted-symbolic");
}

static void on_destroy(GtkWidget *w, gpointer ud) {
    (void)w;
    App *a = ud;
    if (a->timeout_id) g_source_remove(a->timeout_id);
    if (a->dev) { SDL_CloseAudioDevice(a->dev); a->dev = 0; }
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    g_string_free(a->target, TRUE);
    g_string_free(a->typed, TRUE);
    g_free(a->sample);
    g_free(a);
}

/* ---- GitHub footer icon (drawn with Cairo; no image loaders needed) ------ */
static const char GH_PATH[] =
"M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.013 8.013 0 0016 8c0-4.42-3.58-8-8-8z";

/* Minimal SVG path reader: supports M C c A z (all this icon needs). */
static gboolean svg_num(const char **p, double *out) {
    const char *s = *p;
    while (*s == ' ' || *s == ',') s++;
    char *end;
    if (!*s || g_ascii_isalpha(*s)) return FALSE;
    *out = g_ascii_strtod(s, &end);
    if (end == s) return FALSE;
    *p = end;
    return TRUE;
}
static void svg_arc_to(cairo_t *cr, double x0, double y0, double r, int large, int sweep, double x, double y) {
    double mx = (x0 + x) / 2, my = (y0 + y) / 2, dx = x - x0, dy = y - y0;
    double d = sqrt(dx * dx + dy * dy);
    if (d < 1e-9) return;
    if (r < d / 2) r = d / 2;
    double h = sqrt(MAX(0.0, r * r - d * d / 4));
    double ux = -dy / d, uy = dx / d;                 /* unit perpendicular */
    double sgn = (large == sweep) ? -1.0 : 1.0;
    double cx = mx + sgn * h * ux, cy = my + sgn * h * uy;
    double a0 = atan2(y0 - cy, x0 - cx), a1 = atan2(y - cy, x - cx);
    if (sweep) cairo_arc(cr, cx, cy, r, a0, a1); else cairo_arc_negative(cr, cx, cy, r, a0, a1);
}
static void svg_path(cairo_t *cr, const char *p) {
    double cx = 0, cy = 0, v[6];
    char cmd = 0;
    while (*p) {
        while (*p == ' ' || *p == ',') p++;
        if (!*p) break;
        if (g_ascii_isalpha(*p)) { cmd = *p++; if (cmd == 'z' || cmd == 'Z') { cairo_close_path(cr); continue; } }
        switch (cmd) {
        case 'M': if (svg_num(&p, &v[0]) && svg_num(&p, &v[1])) { cairo_move_to(cr, v[0], v[1]); cx = v[0]; cy = v[1]; cmd = 'L'; } else return; break;
        case 'L': if (svg_num(&p, &v[0]) && svg_num(&p, &v[1])) { cairo_line_to(cr, v[0], v[1]); cx = v[0]; cy = v[1]; } else return; break;
        case 'C': case 'c': {
            for (int i = 0; i < 6; i++) if (!svg_num(&p, &v[i])) return;
            double o_x = cmd == 'c' ? cx : 0, o_y = cmd == 'c' ? cy : 0;
            cairo_curve_to(cr, o_x + v[0], o_y + v[1], o_x + v[2], o_y + v[3], o_x + v[4], o_y + v[5]);
            cx = o_x + v[4]; cy = o_y + v[5];
            break; }
        case 'A': {
            double r, rot;
            if (!svg_num(&p, &r) || !svg_num(&p, &v[5]) || !svg_num(&p, &rot)) return;
            while (*p == ' ' || *p == ',') p++;
            int large = *p++ == '1';                  /* flags may be packed: "0016 8" */
            int sweep = *p++ == '1';
            if (!svg_num(&p, &v[0]) || !svg_num(&p, &v[1])) return;
            svg_arc_to(cr, cx, cy, r, large, sweep, v[0], v[1]);
            cx = v[0]; cy = v[1];
            break; }
        default: return;
        }
    }
}
static void gh_draw(GtkDrawingArea *area, cairo_t *cr, int w, int h, gpointer ud) {
    (void)area;
    App *a = ud;
    double s = MIN(w, h) / 16.0;
    cairo_translate(cr, (w - 16 * s) / 2, (h - 16 * s) / 2);
    cairo_scale(cr, s, s);
    cairo_new_path(cr);
    svg_path(cr, GH_PATH);
    if (a->gh_hover) rgb_hex(cr, 0xcba6f7, 1); else rgb_hex(cr, 0x6c7086, 1);
    cairo_fill(cr);
}
static void on_gh_click(GtkGestureClick *g, int n, double x, double y, gpointer ud) {
    (void)g; (void)n; (void)x; (void)y;
    App *a = ud;
    gtk_show_uri(GTK_WINDOW(a->window), CADENCE_REPO_URL, GDK_CURRENT_TIME);
}
static void on_gh_enter(GtkEventControllerMotion *m, double x, double y, gpointer ud) {
    (void)m; (void)x; (void)y; App *a = ud;
    a->gh_hover = TRUE; gtk_widget_queue_draw(a->gh_widget);
}
static void on_gh_leave(GtkEventControllerMotion *m, gpointer ud) {
    (void)m; App *a = ud;
    a->gh_hover = FALSE; gtk_widget_queue_draw(a->gh_widget);
}
static GtkWidget *make_github_link(App *a) {
    a->gh_widget = gtk_drawing_area_new();
    gtk_widget_set_size_request(a->gh_widget, 16, 16);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(a->gh_widget), gh_draw, a, NULL);
    gtk_widget_set_tooltip_text(a->gh_widget, "View source on GitHub");
    gtk_widget_set_cursor_from_name(a->gh_widget, "pointer");

    GtkGesture *click = gtk_gesture_click_new();
    g_signal_connect(click, "released", G_CALLBACK(on_gh_click), a);
    gtk_widget_add_controller(a->gh_widget, GTK_EVENT_CONTROLLER(click));
    GtkEventController *mo = gtk_event_controller_motion_new();
    g_signal_connect(mo, "enter", G_CALLBACK(on_gh_enter), a);
    g_signal_connect(mo, "leave", G_CALLBACK(on_gh_leave), a);
    gtk_widget_add_controller(a->gh_widget, mo);
    return a->gh_widget;
}

static void activate(GtkApplication *gapp, gpointer ud) {
    (void)ud;
    GtkWindow *existing = gtk_application_get_active_window(gapp);
    if (existing) { gtk_window_present(existing); return; }

    App *a = g_new0(App, 1);
    a->target = g_string_new(NULL);
    a->typed  = g_string_new(NULL);
    a->hint_key = a->hint_shift = -1;
    build_layout(a);
    audio_init(a);

    /* theme */
    g_object_set(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", TRUE, NULL);
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css, CSS, -1);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(css),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    a->window = gtk_application_window_new(gapp);
    gtk_window_set_title(GTK_WINDOW(a->window), "Cadence");
    gtk_window_set_default_size(GTK_WINDOW(a->window), 1100, 800);
    g_signal_connect(a->window, "destroy", G_CALLBACK(on_destroy), a);
    g_signal_connect(a->window, "notify::is-active", G_CALLBACK(on_active_changed), a);

    /* header bar: native window buttons are kept; only a sound toggle is added */
    GtkWidget *hb = gtk_header_bar_new();
    GtkWidget *title = gtk_label_new("Cadence");
    gtk_widget_add_css_class(title, "app-title");
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(hb), title);
    a->sound_btn = gtk_toggle_button_new();
    gtk_button_set_icon_name(GTK_BUTTON(a->sound_btn), "audio-volume-high-symbolic");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(a->sound_btn), TRUE);
    gtk_widget_set_tooltip_text(a->sound_btn, "Key sounds");
    gtk_widget_set_focusable(a->sound_btn, FALSE);
    g_signal_connect(a->sound_btn, "toggled", G_CALLBACK(on_sound_toggled), a);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(hb), a->sound_btn);
    gtk_window_set_titlebar(GTK_WINDOW(a->window), hb);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_widget_set_margin_top(vbox, 20);    gtk_widget_set_margin_bottom(vbox, 24);
    gtk_widget_set_margin_start(vbox, 28);  gtk_widget_set_margin_end(vbox, 28);
    gtk_window_set_child(GTK_WINDOW(a->window), vbox);

    /* mode selector */
    GtkWidget *modebar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_widget_add_css_class(modebar, "mode-bar");
    gtk_widget_set_halign(modebar, GTK_ALIGN_CENTER);
    for (int i = 0; i < N_MODES; i++) {
        a->mode_btn[i] = gtk_toggle_button_new_with_label(MODE_LABELS[i]);
        gtk_widget_add_css_class(a->mode_btn[i], "mode-btn");
        gtk_widget_set_focusable(a->mode_btn[i], FALSE);
        if (i) gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(a->mode_btn[i]), GTK_TOGGLE_BUTTON(a->mode_btn[0]));
        gtk_box_append(GTK_BOX(modebar), a->mode_btn[i]);
    }
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(a->mode_btn[0]), TRUE);
    for (int i = 0; i < N_MODES; i++)
        g_signal_connect(a->mode_btn[i], "toggled", G_CALLBACK(on_mode_toggled), a);
    gtk_box_append(GTK_BOX(vbox), modebar);

    /* stats */
    GtkWidget *stats = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 48);
    gtk_widget_set_halign(stats, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(stats), make_stat("WPM",      &a->lbl_wpm,  TRUE));
    gtk_box_append(GTK_BOX(stats), make_stat("ACCURACY", &a->lbl_acc,  FALSE));
    gtk_box_append(GTK_BOX(stats), make_stat("TIME",     &a->lbl_time, FALSE));
    gtk_box_append(GTK_BOX(vbox), stats);

    /* text area: height measured from the real font so exactly 3 lines fit */
    PangoLayout *probe = gtk_widget_create_pango_layout(vbox, "Ag\nAg");
    PangoFontDescription *fd = pango_font_description_from_string(TEXT_FONT);
    pango_layout_set_font_description(probe, fd);
    pango_layout_set_line_spacing(probe, 1.35f);
    int pw, ph; pango_layout_get_pixel_size(probe, &pw, &ph);
    pango_font_description_free(fd); g_object_unref(probe);
    a->text_line_h = ph / 2.0;
    a->text_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(a->text_area, TRUE);
    gtk_widget_set_size_request(a->text_area, 560, (int)(a->text_line_h * 3 + 12));
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(a->text_area), text_draw, a, NULL);
    gtk_box_append(GTK_BOX(vbox), a->text_area);

    a->lbl_msg = gtk_label_new("");
    gtk_widget_add_css_class(a->lbl_msg, "hint");
    gtk_box_append(GTK_BOX(vbox), a->lbl_msg);

    /* keyboard: expands to fill remaining space; kbd_draw scales + centres itself */
    a->kbd_area = gtk_drawing_area_new();
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(a->kbd_area), kbd_draw, a, NULL);
    gtk_widget_set_size_request(a->kbd_area, 700, (int)(700 * KBD_H / KBD_W));
    gtk_widget_set_hexpand(a->kbd_area, TRUE);
    gtk_widget_set_vexpand(a->kbd_area, TRUE);
    gtk_box_append(GTK_BOX(vbox), a->kbd_area);

    /* footer:  [github icon] • Open source project under <license> • <year> */
    GtkWidget *footer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(footer, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(footer), make_github_link(a));
    const char *parts[] = { "\u2022", "Open source project under the " CADENCE_LICENSE " license", "\u2022", CADENCE_YEAR };
    for (guint i = 0; i < G_N_ELEMENTS(parts); i++) {
        GtkWidget *l = gtk_label_new(parts[i]);
        gtk_widget_add_css_class(l, "footer-text");
        gtk_box_append(GTK_BOX(footer), l);
    }
    gtk_box_append(GTK_BOX(vbox), footer);

    a->tick_id = gtk_widget_add_tick_callback(a->kbd_area, kbd_tick, a, NULL);

    /* window‑level keyboard capture: no entry widget, no focus needed */
    GtkEventController *kc = gtk_event_controller_key_new();
    gtk_event_controller_set_propagation_phase(kc, GTK_PHASE_CAPTURE);
    g_signal_connect(kc, "key-pressed",  G_CALLBACK(on_key_pressed),  a);
    g_signal_connect(kc, "key-released", G_CALLBACK(on_key_released), a);
    gtk_widget_add_controller(a->window, kc);

    restart(a, TRUE);
    gtk_window_present(GTK_WINDOW(a->window));
}

int main(int argc, char **argv) {
    g_autoptr(GtkApplication) app = gtk_application_new("com.example.Cadence", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    return g_application_run(G_APPLICATION(app), argc, argv);
}
