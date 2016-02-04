#include "latex.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "escape.h"

static void escape_latex(hoedown_buffer *ob, const uint8_t *source, size_t length)
{
	hoedown_escape_latex(ob, source, length, 0);
}

static void escape_href(hoedown_buffer *ob, const uint8_t *source, size_t length)
{
	hoedown_escape_href(ob, source, length);
}

/********************
 * GENERIC RENDERER *
 ********************/
static void
rndr_blockcode(hoedown_buffer *ob, const hoedown_buffer *text, const hoedown_buffer *lang, const hoedown_renderer_data *data)
{
	char* env = (lang) ? "minted" : "verbatim";

	if (ob->size) hoedown_buffer_putc(ob, '\n');

	hoedown_buffer_printf(ob, "\\begin{%s}", env);
	if (lang) {
		hoedown_buffer_putc(ob, '{');
		escape_latex(ob, lang->data, lang->size);
		hoedown_buffer_putc(ob, '}');
	}
	hoedown_buffer_putc(ob, '\n');

	if (text) hoedown_buffer_put(ob, text->data, text->size);

	hoedown_buffer_printf(ob, "\\end{%s}\n", env);
}

static void
rndr_blockquote(hoedown_buffer *ob, const hoedown_buffer *content, const hoedown_renderer_data *data)
{
	  if (ob->size) hoedown_buffer_putc(ob, '\n');
		HOEDOWN_BUFPUTSL(ob, "\\begin{quotation}\n");
		if (content) hoedown_buffer_put(ob, content->data, content->size);
		HOEDOWN_BUFPUTSL(ob, "\\end{quotation}\n");
}

static int
rndr_codespan(hoedown_buffer *ob, const hoedown_buffer *text, const hoedown_renderer_data *data)
{
	HOEDOWN_BUFPUTSL(ob, "\\texttt{");
	if (text) escape_latex(ob, text->data, text->size);
	hoedown_buffer_putc(ob, '}');
	return 1;
}

static int
rndr_linebreak(hoedown_buffer *ob, const hoedown_renderer_data *data)
{
	HOEDOWN_BUFPUTSL(ob, "\\\\\n");
	return 1;
}

static int
rndr_link(hoedown_buffer *ob, const hoedown_buffer *content, const hoedown_buffer *link, const hoedown_buffer *title, const hoedown_renderer_data *data)
{
	if (link && link->size > 0 && hoedown_buffer_prefix(link, "#") == 0) {
		HOEDOWN_BUFPUTSL(ob, "\\hyperref[");

		if (link && link->size)
			hoedown_buffer_put(ob, link->data + 1, link->size - 1);

		HOEDOWN_BUFPUTSL(ob, "]{");
	}
	else {
		HOEDOWN_BUFPUTSL(ob, "\\hyperref{");

		if (link && link->size)
			escape_href(ob, link->data, link->size);

		HOEDOWN_BUFPUTSL(ob, "}{}{");

		if (title && title->size)
			escape_latex(ob, title->data, title->size);

		HOEDOWN_BUFPUTSL(ob, "}{");
	}

	if (content && content->size) hoedown_buffer_put(ob, content->data, content->size);
	hoedown_buffer_putc(ob, '}');
	return 1;
}

static void
rndr_paragraph(hoedown_buffer *ob, const hoedown_buffer *content, const hoedown_renderer_data *data)
{
	hoedown_latex_renderer_state *state = data->opaque;
	size_t i = 0;

	if (ob->size) hoedown_buffer_putc(ob, '\n');

	if (!content || !content->size)
		return;

	while (i < content->size && isspace(content->data[i])) i++;

	if (i == content->size)
		return;

	if (state->flags & HOEDOWN_LATEX_HARD_WRAP) {
		size_t org;
		while (i < content->size) {
			org = i;
			while (i < content->size && content->data[i] != '\n')
				i++;

			if (i > org)
				hoedown_buffer_put(ob, content->data + org, i - org);

			/*
			 * do not insert a line break if this newline
			 * is the last character on the paragraph
			 */
			if (i >= content->size - 1)
				break;

			rndr_linebreak(ob, data);
			i++;
		}
	} else {
		hoedown_buffer_put(ob, content->data + i, content->size - i);
	}
	hoedown_buffer_putc(ob, '\n');
}

static void
rndr_hrule(hoedown_buffer *ob, const hoedown_renderer_data *data)
{
	  if (ob->size) hoedown_buffer_putc(ob, '\n');
		  HOEDOWN_BUFPUTSL(ob, "\\hrule\\vskip\\baselineskip\n");
}

static int
rndr_image(hoedown_buffer *ob, const hoedown_buffer *link, const hoedown_buffer *title, const hoedown_buffer *alt, const hoedown_renderer_data *data)
{
	if (!link || !link->size) return 0;

	HOEDOWN_BUFPUTSL(ob, "\\begin{figure*}\n\\includegraphics*[width=\\textwidth]{");
	escape_href(ob, link->data, link->size);
	HOEDOWN_BUFPUTSL(ob, "}\n");

	if (alt && alt->size) {
		HOEDOWN_BUFPUTSL(ob, "\\caption{");
		escape_latex(ob, alt->data, alt->size);
		HOEDOWN_BUFPUTSL(ob, "}\n"); }

	if (title && title->size) {
		escape_latex(ob, title->data, title->size);
		hoedown_buffer_putc(ob, '\n'); }

	HOEDOWN_BUFPUTSL(ob, "\\end{figure*}");
	return 1;
}

static int
rndr_raw_html(hoedown_buffer *ob, const hoedown_buffer *text, const hoedown_renderer_data *data)
{
	return 1;
}

static void
rndr_normal_text(hoedown_buffer *ob, const hoedown_buffer *content, const hoedown_renderer_data *data)
{
	if (content)
		escape_latex(ob, content->data, content->size);
}

hoedown_renderer *
hoedown_latex_renderer_new(hoedown_latex_flags render_flags, int nesting_level)
{
	static const hoedown_renderer cb_default = {
		NULL,

		rndr_blockcode,
		rndr_blockquote,
		NULL, /* rndr_header,*/
		rndr_hrule,
		NULL, /* rndr_list,*/
		NULL, /* rndr_listitem,*/
		rndr_paragraph,
		NULL, /* rndr_table,*/
		NULL, /* rndr_table_header,*/
		NULL, /* rndr_table_body,*/
		NULL, /* rndr_tablerow,*/
		NULL, /* rndr_tablecell,*/
		NULL, /* rndr_footnotes,*/
		NULL, /* rndr_footnote_def,*/
		NULL, /* rndr_raw_block,*/

		NULL, /* rndr_autolink,*/
		rndr_codespan,
		NULL, /* rndr_double_emphasis,*/
		NULL, /* rndr_emphasis,*/
		NULL, /* rndr_underline,*/
		NULL, /* rndr_highlight,*/
		NULL, /* rndr_quote,*/
		rndr_image,
		rndr_linebreak,
		rndr_link,
		NULL, /* rndr_triple_emphasis,*/
		NULL, /* rndr_strikethrough,*/
		NULL, /* rndr_superscript,*/
		NULL, /* rndr_footnote_ref,*/
		NULL, /* rndr_math,*/
		rndr_raw_html,

		NULL,
		rndr_normal_text,

		NULL,
		NULL
	};

	hoedown_latex_renderer_state *state;
	hoedown_renderer *renderer;

	/* Prepare the state pointer */
	state = hoedown_malloc(sizeof(hoedown_latex_renderer_state));
	memset(state, 0x0, sizeof(hoedown_latex_renderer_state));

	state->flags = render_flags;
	state->toc_data.nesting_level = nesting_level;

	/* Prepare the renderer */
	renderer = hoedown_malloc(sizeof(hoedown_renderer));
	memcpy(renderer, &cb_default, sizeof(hoedown_renderer));

	renderer->blockhtml = NULL;

	renderer->opaque = state;
	return renderer;
}

void
hoedown_latex_renderer_free(hoedown_renderer *renderer)
{
	free(renderer->opaque);
	free(renderer);
}
