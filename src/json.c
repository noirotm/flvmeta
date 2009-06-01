/***************************************************************************
 *   Copyright (C) 2007 by Rui Maciel   *
 *   rui.maciel@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "json.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <string.h>
#include <wchar.h>


enum LEX_VALUE
{ LEX_MORE = 0,
	LEX_INVALID_CHARACTER,
	LEX_TRUE,
	LEX_FALSE,
	LEX_NULL,
	LEX_BEGIN_OBJECT,
	LEX_END_OBJECT,
	LEX_BEGIN_ARRAY,
	LEX_END_ARRAY,
	LEX_NAME_SEPARATOR,
	LEX_VALUE_SEPARATOR,
	LEX_STRING,
	LEX_NUMBER,
	LEX_ERROR,
	LEX_MEMORY
};


/* rc_string part */

#define RSTRING_INCSTEP 3

struct rui_cstring
{
	char *text;	/*<! char c-string */
	size_t max;	/*<! usable memory allocated to text minus the space for the nul character */
};

typedef struct rui_cstring rcstring;

enum rui_string_error_codes
{ RS_MEMORY, RS_OK = 1, RS_UNKNOWN };

typedef enum rui_string_error_codes rstring_code;


rcstring *
rcs_create (size_t length)
{
	rcstring *rcs;
	rcs = malloc (sizeof (rcstring));	/* allocates memory for a struct rcstring */
	if (rcs == NULL)
		return NULL;

	rcs->max = length;

	rcs->text = calloc (rcs->max + 1, sizeof (char));
	if (rcs->text == NULL)
	{
		free (rcs);
		return NULL;
	}

	return rcs;
}


void
rcs_free (rcstring ** rcs)
{
	assert (rcs != NULL);
	if (*rcs != NULL)
	{
		if ((*rcs)->text != NULL)
		{
			free ((*rcs)->text);
			(*rcs)->text = NULL;
		}
		free (*rcs);
		*rcs = NULL;
	}

}


rstring_code
rcs_resize (rcstring * rcs, size_t length)
{
	char *temp;
	assert (rcs != NULL);

	temp = realloc (rcs->text, sizeof (char) * (length + 1));	/* length plus L'\0' */
	if (temp == NULL)
	{
		free (rcs);
		return RS_MEMORY;
	}
	rcs->text = temp;
	rcs->max = length;
	rcs->text[rcs->max] = L'\0';
	return RS_OK;
}


rstring_code
rcs_catcs (rcstring * pre, const char *pos, const size_t length)
{
	size_t pre_length;

	assert (pre != NULL);
	assert (pos != NULL);

	pre_length = strlen (pre->text);

	if (pre->max < pre_length + length)
	{
		if (rcs_resize (pre, pre_length + length + 5) != RS_OK)
			return RS_MEMORY;
	}
	strncpy (pre->text + pre_length, pos, length);
	pre->text[pre_length + length] = '\0';
	return RS_OK;
}


rstring_code
rcs_catc (rcstring * pre, const char c)
{
	size_t pre_length;

	assert (pre != NULL);

	pre_length = strlen (pre->text);
	if (pre->max <= pre_length)
	{
		pre->max += RSTRING_INCSTEP;
		if (rcs_resize (pre, pre->max) != RS_OK)
			return RS_MEMORY;
	}
	pre->text[pre_length] = c;
	pre->text[pre_length + 1] = '\0';
	return RS_OK;
}


char *
rcs_unwrap (rcstring * rcs)
{
	char *out;
	assert (rcs != NULL);

	if (rcs->text == NULL)
		out = NULL;
	else
		out = realloc (rcs->text, sizeof (char) * (strlen (rcs->text) + 1));

	free (rcs);
	return out;
}



size_t
rcs_length (rcstring * rcs)
{
	/*TODO account for UTF8 */
	assert (rcs != NULL);
	return strlen (rcs->text);
}


/* end of rc_string part */



json_t *
json_new_value (const enum json_value_type type)
{
	json_t *new_object;
	/* allocate memory to the new object */
	new_object = malloc (sizeof (json_t));
	if (new_object == NULL)
		return NULL;

	/* initialize members */
	new_object->text = NULL;
	new_object->parent = NULL;
	new_object->child = NULL;
	new_object->child_end = NULL;
	new_object->previous = NULL;
	new_object->next = NULL;
	new_object->type = type;
	return new_object;
}


json_t *
json_new_string (const char *text)
{
	json_t *new_object;
	size_t length;

	assert (text != NULL);

	/* allocate memory for the new object */
	new_object = malloc (sizeof (json_t));
	if (new_object == NULL)
		return NULL;

	/* initialize members */
	length = strlen (text) + 1;
	new_object->text = calloc (sizeof (char), length);
	if (new_object->text == NULL)
	{
		free (new_object);
		return NULL;
	}
	strncpy (new_object->text, text, length);
	new_object->parent = NULL;
	new_object->child = NULL;
	new_object->child_end = NULL;
	new_object->previous = NULL;
	new_object->next = NULL;
	new_object->type = JSON_STRING;
	return new_object;
}


json_t *
json_new_number (const char *text)
{
	json_t *new_object;
	size_t length;

	assert (text != NULL);

	/* allocate memory for the new object */
	new_object = malloc (sizeof (json_t));
	if (new_object == NULL)
		return NULL;

	/* initialize members */
	length = strlen (text) + 1;
	new_object->text = calloc (sizeof (char), length);
	if (new_object->text == NULL)
	{
		free (new_object);
		return NULL;
	}
	strncpy (new_object->text, text, length);
	new_object->parent = NULL;
	new_object->child = NULL;
	new_object->child_end = NULL;
	new_object->previous = NULL;
	new_object->next = NULL;
	new_object->type = JSON_NUMBER;
	return new_object;
}


json_t *
json_new_object (void)
{
	return json_new_value (JSON_OBJECT);
}


json_t *
json_new_array (void)
{
	return json_new_value (JSON_ARRAY);
}


json_t *
json_new_null (void)
{
	return json_new_value (JSON_NULL);
}


json_t *
json_new_true (void)
{
	return json_new_value (JSON_TRUE);
}


json_t *
json_new_false (void)
{
	return json_new_value (JSON_FALSE);
}


void
json_free_value (json_t ** value)
{
	assert (value != NULL);
	assert ((*value) != NULL);

	/* free each and every child node */
	if ((*value)->child != NULL)
	{
		json_t *i, *j;
		i = (*value)->child_end;
		while (i != NULL)
		{
			j = i->previous;
			json_free_value (&i);
			i = j;
		}
	}

	/* fixing sibling linked list connections */
	if ((*value)->previous && (*value)->next)
	{
		(*value)->previous->next = (*value)->next;
		(*value)->next->previous = (*value)->previous;
	}
	else
	{
		if ((*value)->previous)
		{
			(*value)->previous->next = NULL;
		}
		if ((*value)->next)
		{
			(*value)->next->previous = NULL;
		}
	}

	/*fixing parent node connections */
	if ((*value)->parent)
	{
		if ((*value)->parent->child == (*value))
		{
			if ((*value)->next)
			{
				(*value)->parent->child = (*value)->next;	/* the parent node always points to the first node */
			}
			else
			{
				if ((*value)->previous)
					(*value)->parent->child = (*value)->next;	/* the parent node always points to the first node */
				(*value)->parent->child = NULL;
			}
		}
	}

	/*finally, freeing the memory allocated for this value */
	if ((*value)->text != NULL)
	{
		free ((*value)->text);
	}
	free (*value);		/* the json value */
	(*value) = NULL;
}


enum json_error
json_insert_child (json_t * parent, json_t * child)
{
	/*TODO change the child list from FIFO to LIFO, in order to get rid of the child_end pointer */
	assert (parent != NULL);	/* the parent must exist */
	assert (child != NULL);	/* the child must exist */
	assert (parent != child);	/* parent and child must not be the same. if they are, it will enter an infinite loop */

	/* enforce tree structure correctness */
	switch (parent->type)
	{
	case JSON_STRING:
		/* a string accepts every JSON type as a child value */
		/* therefore, the sanity check must be performed on the child node */
		switch (child->type)
		{
		case JSON_STRING:
		case JSON_NUMBER:
		case JSON_TRUE:
		case JSON_FALSE:
		case JSON_NULL:
			if (child->child != NULL)
				return JSON_BAD_TREE_STRUCTURE;
			break;

		case JSON_OBJECT:
		case JSON_ARRAY:
			break;

		default:
			return JSON_BAD_TREE_STRUCTURE;	/* this part should never be reached */
			break;
		}
		break;

	case JSON_OBJECT:	/* JSON objects may only accept JSON string objects which already have child nodes of their own */
		if (child->type != JSON_STRING)
			return JSON_BAD_TREE_STRUCTURE;
		break;

	case JSON_ARRAY:
		switch (child->type)
		{
		case JSON_STRING:
		case JSON_TRUE:
		case JSON_FALSE:
		case JSON_NULL:
		case JSON_NUMBER:
			if (child->child)
				return JSON_BAD_TREE_STRUCTURE;
			break;

		case JSON_OBJECT:
		case JSON_ARRAY:
			break;
		default:
			return JSON_BAD_TREE_STRUCTURE;
		}
		break;

	default:
		return JSON_BAD_TREE_STRUCTURE;
	}

	child->parent = parent;
	if (parent->child)
	{
		child->previous = parent->child_end;
		parent->child_end->next = child;
		parent->child_end = child;
	}
	else
	{
		parent->child = child;
		parent->child_end = child;
	}

	return JSON_OK;
}


enum json_error
json_insert_pair_into_object (json_t * parent, const char *text_label, json_t * value)
{
	enum json_error error;
	json_t *label;
	/* verify if the parameters are valid */
	assert (parent != NULL);
	assert (text_label != NULL);
	assert (value != NULL);
	assert (parent != value);

	/* enforce type coherence */
	assert (parent->type == JSON_OBJECT);


	/* create label json_value */
	label = json_new_string (text_label);
	if (label == NULL)
		return JSON_MEMORY;

	/*insert value and check for error */
	error = json_insert_child (label, value);
	if (error != JSON_OK)
		return error;
	/*insert value and check for error */
	error = json_insert_child (parent, label);
	if (error != JSON_OK)
		return error;

	return JSON_OK;
}


enum json_error
json_tree_to_string (json_t * root, char **text)
{
	json_t *cursor;
	rcstring *output;
	assert (root != NULL);
	assert (text != NULL);

	cursor = root;
	/* set up the output and temporary rwstrings */
	output = rcs_create (5);

	/* start the convoluted fun */
      state1:			/* open value */
	{
		if ((cursor->previous) && (cursor != root))	/*if cursor is children and not root than it is a followup sibling */
		{
			/* append comma */
			if (rcs_catc (output, ',') != RS_OK)
			{
				return JSON_MEMORY;
			}
		}
		switch (cursor->type)
		{
		case JSON_STRING:
			/* append the "text"\0, which means 1 + wcslen(cursor->text) + 1 + 1 */
			/* set the new output size */
			if (rcs_catc (output, '\"') != RS_OK)
			{
				return JSON_MEMORY;
			}
			if (rcs_catcs (output, cursor->text, strlen (cursor->text)) != RS_OK)
			{
				return JSON_MEMORY;
			}
			if (rcs_catc (output, '\"') != RS_OK)
			{
				return JSON_MEMORY;
			}

			if (cursor->parent != NULL)
			{
				if (cursor->parent->type == JSON_OBJECT)	/* cursor is label in label:value pair */
				{
					/* error checking: if parent is object and cursor is string then cursor must have a single child */
					if (cursor->child != NULL)
					{
						if (rcs_catc (output, ':') != RS_OK)
						{
							return JSON_MEMORY;
						}
					}
					else
					{
						/* malformed document tree: label without value in label:value pair */
						rcs_free (&output);
						text = NULL;
						return JSON_BAD_TREE_STRUCTURE;
					}
				}
			}
			else	/* does not have a parent */
			{
				if (cursor->child != NULL)	/* is root label in label:value pair */
				{
					if (rcs_catc (output, ':') != RS_OK)
					{
						return JSON_MEMORY;
					}
				}
				else
				{
					/* malformed document tree: label without value in label:value pair */
					rcs_free (&output);
					text = NULL;
					return JSON_BAD_TREE_STRUCTURE;
				}
			}
			break;

		case JSON_NUMBER:
			/* must not have any children */
			/* set the new size */
			if (rcs_catcs (output, cursor->text, strlen (cursor->text)) != RS_OK)
			{
				return JSON_MEMORY;
			}
			goto state2;	/* close value */
			break;

		case JSON_OBJECT:
			if (rcs_catc (output, '{') != RS_OK)
			{
				return JSON_MEMORY;
			}

			if (cursor->child)
			{
				cursor = cursor->child;
				goto state1;	/* open value */
			}
			else
			{
				goto state2;	/* close value */
			}
			break;

		case JSON_ARRAY:
			if (rcs_catc (output, '[') != RS_OK)
			{
				return JSON_MEMORY;
			}

			if (cursor->child != NULL)
			{
				cursor = cursor->child;
				goto state1;
			}
			else
			{
				goto state2;	/* close value */
			}
			break;

		case JSON_TRUE:
			/* must not have any children */
			if (rcs_catcs (output, "true", 4) != RS_OK)
			{
				return JSON_MEMORY;
			}
			goto state2;	/* close value */
			break;

		case JSON_FALSE:
			/* must not have any children */
			if (rcs_catcs (output, "false", 5) != RS_OK)
			{
				return JSON_MEMORY;
			}
			goto state2;	/* close value */
			break;

		case JSON_NULL:
			/* must not have any children */
			if (rcs_catcs (output, "null", 4) != RS_OK)
			{
				return JSON_MEMORY;
			}
			goto state2;	/* close value */
			break;

		default:
			goto error;
		}
		if (cursor->child)
		{
			cursor = cursor->child;
			goto state1;	/* open value */
		}
		else
		{
			/* does not have any children */
			goto state2;	/* close value */
		}
	}

      state2:			/* close value */
	{
		switch (cursor->type)
		{
		case JSON_OBJECT:
			if (rcs_catc (output, '}') != RS_OK)
			{
				return JSON_MEMORY;
			}
			break;

		case JSON_ARRAY:
			if (rcs_catc (output, ']') != RS_OK)
			{
				return JSON_MEMORY;
			}
			break;

		case JSON_STRING:
			break;
		case JSON_NUMBER:
			break;
		case JSON_TRUE:
			break;
		case JSON_FALSE:
			break;
		case JSON_NULL:
			break;
		default:
			goto error;
		}
		if ((cursor->parent == NULL) || (cursor == root))
		{
			goto end;
		}
		else if (cursor->next)
		{
			cursor = cursor->next;
			goto state1;	/* open value */
		}
		else
		{
			cursor = cursor->parent;
			goto state2;	/* close value */
		}
	}

      error:
	{
		rcs_free (&output);
		return JSON_UNKNOWN_PROBLEM;
	}

      end:
	{
		*text = rcs_unwrap(output);
		return JSON_OK;
	}
}


void
json_strip_white_spaces (char *text)
{
	size_t in, out, length;
	int state;

	assert (text != NULL);

	in = 0;
	out = 0;
	length = strlen (text);
	state = 0;		/* possible states: 0 -> document, 1 -> inside a string */

	while (in < length)
	{
		switch (text[in])
		{
		case '\x20':	/* space */
		case '\x09':	/* horizontal tab */
		case '\x0A':	/* line feed or new line */
		case '\x0D':	/* Carriage return */
			if (state == 1)
			{
				text[out++] = text[in];
			}
			break;

		case '\"':
			switch (state)
			{
			case 0:	/* not inside a JSON string */
				state = 1;
				break;

			case 1:	/* inside a JSON string */
				if (text[in - 1] != '\\')
				{
					state = 0;
				}
				break;

			default:
				assert (0);
			}
			text[out++] = text[in];
			break;

		default:
			text[out++] = text[in];
		}
		++in;
	}
	text[out] = '\0';
}


char *
json_format_string (char *text)
{
	size_t pos = 0;
	unsigned int indentation = 0;	/* the current indentation level */
	unsigned int i;		/* loop iterator variable */
	char loop;

	rcstring *output;

	output = rcs_create (strlen (text));
	while (pos < strlen (text))
	{
		switch (text[pos])
		{
		case '\x20':
		case '\x09':
		case '\x0A':
		case '\x0D':	/* JSON insignificant white spaces */
			pos++;
			break;

		case '{':
			indentation++;
			rcs_catc (output, '{');
			for (i = 0; i < indentation; i++)
			{
				rcs_catc (output, '\t');
			}
			pos++;
			break;

		case '}':
			indentation--;
			rcs_catc (output, '\n');
			for (i = 0; i < indentation; i++)
			{
				rcs_catc (output, '\t');
			}
			rcs_catc (output, '}');
			pos++;
			break;

		case ':':
			rcs_catcs (output, ": ", 2);
			pos++;
			break;

		case ',':
			rcs_catcs (output, ",\n", 2);
			for (i = 0; i < indentation; i++)
			{
				rcs_catc (output, '\t');
			}
			pos++;
			break;

		case '\"':	/* open string */
			rcs_catc (output, text[pos]);
			pos++;
			loop = 1;	/* inner string loop trigger is enabled */
			while (loop)	/* parse the inner part of the string   ///TODO rethink this loop */
			{
				if (text[pos] == '\\')	/* escaped sequence */
				{
					rcs_catc (output, '\\');
					pos++;
					if (text[pos] == '\"')	/* don't consider a \" escaped sequence as an end of string */
					{
						rcs_catc (output, '\"');
						pos++;
					}
				}
				else if (text[pos] == '\"')	/* reached end of string */
				{
					loop = 0;
				}

				rcs_catc (output, text[pos]);

				pos++;
				if (pos >= strlen (text))
				{
					loop = 0;
				}
			}
			break;

		default:
			rcs_catc (output, text[pos]);
			pos++;
			break;
		}
	}

	return rcs_unwrap (output);
}


char *
json_escape (char * text)
{
	rcstring *output;
	size_t i, length;
	char buffer[6];
	/* check if pre-conditions are met */
	assert (text != NULL);

	/* defining the temporary variables */
	length = strlen(text);
	output = rcs_create (length);
	if (output == NULL)
		return NULL;
	for (i = 0; i < length; i++)
	{
		if (text[i] == '\\')
		{
			rcs_catcs (output, "\\\\", 2);
		}
		else if (text[i] == '\"')
		{
			rcs_catcs (output, "\\\"", 2);
		}
		else if (text[i] == '/')
		{
			rcs_catcs (output, "\\/", 2);
		}
		else if (text[i] == '\b')
		{
			rcs_catcs (output, "\\b", 2);
		}
		else if (text[i] == '\f')
		{
			rcs_catcs (output, "\\f", 2);
		}
		else if (text[i] == '\n')
		{
			rcs_catcs (output, "\\n", 2);
		}
		else if (text[i] == '\r')
		{
			rcs_catcs (output, "\\r", 2);
		}
		else if (text[i] == '\t')
		{
			rcs_catcs (output, "\\t", 2);
		}
		else if (text[i] < 0)	/* non-BMP character */
		{
			rcs_catc (output, text[i]);
		}
		else if (text[i] < 0x20)
		{
			sprintf(buffer,"\\u%4.4x",text[i]);
			rcs_catcs(output,buffer,6);
		}
		else
		{
			rcs_catc (output, text[i]);
		}
	}
	return rcs_unwrap (output);
}


void
json_jpi_init (struct json_parsing_info *jpi)
{
	assert (jpi != NULL);
	jpi->state = 0;
	jpi->lex_state = 0;
	jpi->lex_text = NULL;
	jpi->p = NULL;
	jpi->cursor = NULL;
	jpi->string_length_limit_reached = 0;
}


int
lexer (char *buffer, char **p, unsigned int *state, rcstring ** text)
{
	assert (buffer != NULL);
	assert (p != NULL);
	assert (state != NULL);
	assert (text != NULL);
	if (*p == NULL)
		*p = buffer;

	while (**p != '\0')
	{
		switch (*state)
		{

		case 0:	/* Root document */
			{
				switch (*(*p)++)
				{
				case '\x20':	/* space */
				case '\x09':	/* horizontal tab */
				case '\x0A':	/* line feed or new line */
				case '\x0D':	/* Carriage return */
					break;

				case '{':
					return LEX_BEGIN_OBJECT;
				case '}':
					return LEX_END_OBJECT;
				case '[':
					return LEX_BEGIN_ARRAY;
				case ']':
					return LEX_END_ARRAY;
				case ':':
					return LEX_NAME_SEPARATOR;
				case ',':
					return LEX_VALUE_SEPARATOR;

				case '\"':
					*text = rcs_create (5);
					if (*text == NULL)
						return LEX_MEMORY;
					*state = 1;	/* inside a JSON string */
					break;

				case 't':
					*state = 7;	/* true: 1 */
					break;

				case 'f':
					*state = 10;	/* false: 1 */
					break;

				case 'n':
					*state = 14;	/* false: 1 */
					break;

				case '-':
					*text = rcs_create (5);
					if (*text == NULL)
						return LEX_MEMORY;
					if (rcs_catc (*text, '-') != RS_OK)
						return LEX_MEMORY;
					*state = 17;	/* number: '0' */
					break;

				case '0':
					*text = rcs_create (5);
					if (*text == NULL)
						return LEX_MEMORY;
					if (rcs_catc (*text, '0') != RS_OK)
						return LEX_MEMORY;
					*state = 18;	/* number: '0' */
					break;

				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					*text = rcs_create (5);
					if (*text == NULL)
						return LEX_MEMORY;
					if (rcs_catc (*text, *(*p - 1)) != RS_OK)
						return LEX_MEMORY;
					*state = 19;	/* number: decimal followup */
					break;


				default:
					return LEX_INVALID_CHARACTER;
				}
			}
			break;

		case 1:	/* inside a JSON string */
			{
				assert (*text != NULL);
				switch (**p)
				{
				case '\"':	/* close JSON string */
					/* it is expected that, in the routine that calls this function, text is set to NULL */
					*state = 0;
					++*p;
					return LEX_STRING;
					break;

				case '\\':
					if (rcs_catc (*text, '\\') != RS_OK)
						return LEX_MEMORY;
					*state = 2;	/* inside a JSON string: start escape sequence */
					break;

				default:
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
				}
				++*p;
			}
			break;

		case 2:	/* inside a JSON string: start escape sequence */
			{
				assert (*text != NULL);
				switch (**p)
				{
				case '\\':
				case '\"':
				case '/':
				case 'b':
				case 'f':
				case 'n':
				case 'r':
				case 't':
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					*state = 1;	/* inside a JSON string */
					break;

				case 'u':
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					*state = 3;	/* inside a JSON string: escape unicode */
					break;

				default:
					return LEX_INVALID_CHARACTER;
				}
				++*p;
			}
			break;

		case 3:	/*inside a JSON string: escape unicode */
			{
				assert (*text != NULL);
				if ((**p >= 'a') && (**p <= 'e'))
				{
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					*state = 4;	/* inside a JSON string: escape unicode */
				}
				else if ((**p >= 'A') && (**p <= 'E'))
				{
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					*state = 4;	/* inside a JSON string: escape unicode */
				}
				else if ((**p >= '0') && (**p <= '9'))
				{
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					*state = 4;	/* inside a JSON string: escape unicode */
				}
				else
					return LEX_INVALID_CHARACTER;
				++*p;
			}
			break;

		case 4:	/* inside a JSON string: escape unicode */
			{
				assert (*text != NULL);
				if ((**p >= 'a') && (**p <= 'e'))
				{
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					*state = 5;	/* inside a JSON string: escape unicode */
				}
				else if ((**p >= 'A') && (**p <= 'E'))
				{
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					*state = 5;	/* inside a JSON string: escape unicode */
				}
				else if ((**p >= '0') && (**p <= '9'))
				{
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					*state = 5;	/* inside a JSON string: escape unicode */
				}
				else
					return LEX_INVALID_CHARACTER;
				++*p;
			}

		case 5:	/* inside a JSON string: escape unicode */
			{
				assert (*text != NULL);
				if ((**p >= 'a') && (**p <= 'e'))
				{
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					*state = 6;	/* inside a JSON string: escape unicode */
				}
				else if ((**p >= 'A') && (**p <= 'E'))
				{
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					*state = 6;	/* inside a JSON string: escape unicode */
				}
				else if ((**p >= '0') && (**p <= '9'))
				{
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					*state = 6;	/* inside a JSON string: escape unicode */
				}
				else
					return LEX_INVALID_CHARACTER;
				++*p;
			}
			break;

		case 6:	/* inside a JSON string: escape unicode */
			{
				assert (*text != NULL);
				if ((**p >= 'a') && (**p <= 'e'))
				{
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					*state = 1;	/* inside a JSON string: escape unicode */
				}
				else if ((**p >= 'A') && (**p <= 'E'))
				{
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					*state = 1;	/* inside a JSON string: escape unicode */
				}
				else if ((**p >= '0') && (**p <= '9'))
				{
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					*state = 1;	/* inside a JSON string: escape unicode */
				}
				else
					return LEX_INVALID_CHARACTER;
				++*p;
			}
			break;

		case 7:	/* true: 1 */
			{
				switch (*(*p)++)
				{
				case 'r':
					*state = 8;
					break;
				default:
					return LEX_INVALID_CHARACTER;
					break;
				}
			}
			break;

		case 8:	/* true: 2 */
			{
				switch (*(*p)++)
				{
				case 'u':
					*state = 9;
					break;
				default:
					return LEX_INVALID_CHARACTER;
					break;
				}
			}
			break;

		case 9:	/* true: 3 */
			{
				switch (*(*p)++)
				{
				case 'e':
					*state = 0;
					return LEX_TRUE;
					break;
				default:
					return LEX_INVALID_CHARACTER;
					break;
				}
			}
			break;

		case 10:	/* false: 1 */
			{
				switch (*(*p)++)
				{
				case 'a':
					*state = 11;
					break;
				default:
					return LEX_INVALID_CHARACTER;
					break;
				}
			}
			break;

		case 11:	/* false: 2 */
			{
				switch (*(*p)++)
				{
				case 'l':
					*state = 12;
					break;
				default:
					return LEX_INVALID_CHARACTER;
					break;
				}
			}
			break;

		case 12:	/* false: 3 */
			{
				switch (*(*p)++)
				{
				case 's':
					*state = 13;
					break;
				default:
					return LEX_INVALID_CHARACTER;
					break;
				}
			}
			break;

		case 13:	/* false: 4 */
			{
				switch (*(*p)++)
				{
				case 'e':
					*state = 0;
					return LEX_FALSE;
					break;
				default:
					return LEX_INVALID_CHARACTER;
					break;
				}
			}
			break;

		case 14:	/* null: 1 */
			{
				switch (*(*p)++)
				{
				case 'u':
					*state = 15;
					break;
				default:
					return LEX_INVALID_CHARACTER;
					break;
				}
			}
			break;

		case 15:	/* null: 2 */
			{
				switch (*(*p)++)
				{
				case 'l':
					*state = 16;
					break;
				default:
					return LEX_INVALID_CHARACTER;
					break;
				}
			}
			break;

		case 16:	/* null: 3 */
			{
				switch (*(*p)++)
				{
				case 'l':
					*state = 0;
					return LEX_NULL;
					break;
				default:
					return LEX_INVALID_CHARACTER;
					break;
				}
			}
			break;

		case 17:	/* number: minus sign */
			{
				assert (*text != NULL);
				switch (**p)
				{
				case '0':
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					++*p;
					*state = 18;	/* number: '0' */
					break;

				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					++*p;
					*state = 19;	/* number: decimal followup */
					break;

				default:
					return LEX_INVALID_CHARACTER;
					break;
				}
			}
			break;

		case 18:	/* number: '0' */
			{
				assert (*text != NULL);
				switch (**p)
				{
				case '\x20':	/* space */
				case '\x09':	/* horizontal tab */
				case '\x0A':	/* line feed or new line */
				case '\x0D':	/* Carriage return */
					++*p;
				case ']':
				case '}':
				case ',':
					*state = 0;
					return LEX_NUMBER;
					break;

				case '.':
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					/*TODO finish this state */
					++*p;
					*state = 20;	/* number: frac start */
					break;

				case 'e':
				case 'E':
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					++*p;
					*state = 22;    /* number: exp start */
					break;	

				default:
					return LEX_INVALID_CHARACTER;
					break;
				}
			}
			break;

		case 19:	/* number: int followup */
			{
				assert (*text != NULL);
				switch (**p)
				{
				case '\x20':	/* space */
				case '\x09':	/* horizontal tab */
				case '\x0A':	/* line feed or new line */
				case '\x0D':	/* Carriage return */
					++*p;
				case ']':
				case '}':
				case ',':
					*state = 0;
					return LEX_NUMBER;
					break;

				case '.':
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					++*p;
					*state = 20;	/* number: frac start */
					break;

				case 'e':
				case 'E':
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					++*p;
					*state = 22;	/* number: exp start */
					break;

				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					++*p;
					break;

				default:
					return LEX_INVALID_CHARACTER;
					break;
				}
			}
			break;

		case 20:	/* number: frac start */
			{
				assert (*text != NULL);
				switch (**p)
				{
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					++*p;
					*state = 21;	/* number: frac continue */
					break;

				default:
					return LEX_INVALID_CHARACTER;
					break;
				}
			}
			break;

		case 21:	/* number: frac continue */
			{
				assert (*text != NULL);
				switch (**p)
				{
				case '\x20':	/* space */
				case '\x09':	/* horizontal tab */
				case '\x0A':	/* line feed or new line */
				case '\x0D':	/* Carriage return */
					++*p;
				case ']':
				case '}':
				case ',':
					*state = 0;
					return LEX_NUMBER;
					break;

				case 'e':
				case 'E':
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					++*p;
					*state = 22;	/* number: exp start */
					break;

				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					++*p;
					break;

				default:
					return LEX_INVALID_CHARACTER;
					break;
				}
			}
			break;

		case 22:	/* number: exp start */
			{
				assert (*text != NULL);
				switch (**p)
				{
				case '-':
				case '+':
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					++*p;
					*state = 23;	/* number: exp continue */
					break;

				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					++*p;
					*state = 24;	/* number: exp end */
					break;

				default:
					return LEX_INVALID_CHARACTER;
					break;
				}
			}
			break;

		case 23:	/* number: exp continue */
			{
				assert (*text != NULL);
				switch (**p)
				{
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					++*p;
					*state = 24;	/* number: exp end */
					break;

				default:
					return LEX_INVALID_CHARACTER;
					break;
				}
			}
			break;

		case 24:	/* number: exp end */
			{
				assert (*text != NULL);
				switch (**p)
				{
				case '\x20':	/* space */
				case '\x09':	/* horizontal tab */
				case '\x0A':	/* line feed or new line */
				case '\x0D':	/* Carriage return */
					++*p;
				case ']':
				case '}':
				case ',':
					*state = 0;
					return LEX_NUMBER;
					break;

				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					if (rcs_catc (*text, **p) != RS_OK)
						return LEX_MEMORY;
					++*p;
					break;

				default:
					return LEX_INVALID_CHARACTER;
					break;
				}
			}
			break;

		default:
			printf ("*state missing: %d\n", *state);
			return LEX_INVALID_CHARACTER;
		}

	}

	*p = NULL;
	return LEX_MORE;
}


enum json_error
json_parse_fragment (struct json_parsing_info *info, char *buffer)
{
	json_t *temp = NULL;

	assert (info != NULL);
	assert (buffer != NULL);

	info->p = buffer;
	while (*info->p != '\0')
	{
		switch (info->state)
		{
		case 0:	/* starting point */
			{
				switch (lexer (buffer, &info->p, &info->lex_state, (rcstring **) & info->lex_text))
				{
				case LEX_BEGIN_OBJECT:
					info->state = 1;	/* begin object */
					break;

				case LEX_BEGIN_ARRAY:
					info->state = 7;	/* begin array */
					break;

				default:
					printf ("state %d: defaulted\n", info->state);
					return JSON_MALFORMED_DOCUMENT;
					break;
				}
			}
			break;

		case 1:	/* open object */
			{
				if (info->cursor == NULL)
				{
					if ((info->cursor = json_new_object ()) == NULL)
					{
						return JSON_MEMORY;
					}
				}
				else
				{
					/* perform tree sanity check */
					assert ((info->cursor->type == JSON_STRING) || (info->cursor->type == JSON_ARRAY));

					if ((temp = json_new_object ()) == NULL)
					{
						return JSON_MEMORY;
					}
					if (json_insert_child (info->cursor, temp) != JSON_OK)
					{
						return JSON_UNKNOWN_PROBLEM;
					}
					info->cursor = temp;
					temp = NULL;
				}
				info->state = 2;	/* just entered an object */
			}
			break;

		case 2:	/* opened object */
			{
				/*perform tree sanity checks */
				assert (info->cursor != NULL);
				assert (info->cursor->type == JSON_OBJECT);

				switch (lexer (buffer, &info->p, &info->lex_state, (rcstring **) & info->lex_text))
				{
				case LEX_STRING:
					if ((temp = json_new_value (JSON_STRING)) == NULL)
						return JSON_MEMORY;
					temp->text = rcs_unwrap ((rcstring *) info->lex_text), info->lex_text = NULL;
					if (json_insert_child (info->cursor, temp) != JSON_OK)
					{
						/*TODO return value according to the value returned from json_insert_child() */
						return JSON_UNKNOWN_PROBLEM;
					}
					info->cursor = temp;
					temp = NULL;
					info->state = 5;	/* label, pre label:value separator */
					break;

				case LEX_END_OBJECT:
					if (info->cursor->parent == NULL)
					{
						info->state = 99;	/* finished document. only accept whitespaces until EOF */
					}
					else
					{
						info->cursor = info->cursor->parent;
						switch (info->cursor->type)
						{
						case JSON_STRING:
							/* perform tree sanity checks */
							assert (info->cursor->parent != NULL);

							info->cursor = info->cursor->parent;
							if (info->cursor->type != JSON_OBJECT)
							{
								return JSON_BAD_TREE_STRUCTURE;
							}
							else
							{
								info->state = 3;	/* finished adding a field to an object */
							}
							break;

						case JSON_ARRAY:
							info->state = 9;
							break;

						default:
							return JSON_BAD_TREE_STRUCTURE;
						}
					}
					break;

				case LEX_MORE:
					return JSON_INCOMPLETE_DOCUMENT;
					break;

				default:
					printf ("state %d: defaulted\n", info->state);
					return JSON_MALFORMED_DOCUMENT;
					break;
				}
			}
			break;

		case 3:	/* finished adding a field to an object */
			{
				/*perform tree sanity checks */
				assert (info->cursor != NULL);
				assert (info->cursor->type == JSON_OBJECT);

				switch (lexer (buffer, &info->p, &info->lex_state, (rcstring **) & info->lex_text))
				{
				case LEX_VALUE_SEPARATOR:
					info->state = 4;	/* sibling, post-object */
					break;

				case LEX_END_OBJECT:
					if (info->cursor->parent == NULL)
					{
						info->state = 99;	/* parse until EOF */
					}
					else
					{
						info->cursor = info->cursor->parent;
						switch (info->cursor->type)
						{
						case JSON_STRING:
							/* perform tree sanity checks */
							assert (info->cursor->parent != NULL);

							info->cursor = info->cursor->parent;
							if (info->cursor->type != JSON_OBJECT)
							{
								return JSON_BAD_TREE_STRUCTURE;
							}
							else
							{
								info->state = 3;	/* finished adding a field to an object */
							}
							break;

						case JSON_ARRAY:
							info->state = 9;
							break;

						default:
							return JSON_BAD_TREE_STRUCTURE;
						}
					}
					break;

				case LEX_MORE:
					return JSON_INCOMPLETE_DOCUMENT;
					break;

				default:
					printf ("state %d: defaulted\n", info->state);
					return JSON_MALFORMED_DOCUMENT;
					break;
				}
			}
			break;

		case 4:	/* sibling, post-object */
			{
				assert (info->cursor != NULL);
				assert (info->cursor->type == JSON_OBJECT);

				switch (lexer (buffer, &info->p, &info->lex_state, (rcstring **) & info->lex_text))
				{
				case LEX_STRING:
					if ((temp = json_new_value (JSON_STRING)) == NULL)
						return JSON_MEMORY;
					temp->text = rcs_unwrap ((rcstring *) info->lex_text), info->lex_text = NULL;
					if (json_insert_child (info->cursor, temp) != JSON_OK)
					{
						return JSON_UNKNOWN_PROBLEM;
					}
					info->cursor = temp;
					temp = NULL;
					info->state = 5;
					break;

				case LEX_MORE:
					return JSON_INCOMPLETE_DOCUMENT;
					break;

				default:
					printf ("state %d: defaulted\n", info->state);
					return JSON_MALFORMED_DOCUMENT;
					break;
				}
			}
			break;

		case 5:	/* label, pre name separator */
			{
				/* perform tree sanity checks */
				assert (info->cursor != NULL);
				assert (info->cursor->type == JSON_STRING);

				switch (lexer (buffer, &info->p, &info->lex_state, (rcstring **) & info->lex_text))
				{
				case LEX_NAME_SEPARATOR:
					info->state = 6;	/* label, pos label:value separator */
					break;

				case LEX_MORE:
					return JSON_INCOMPLETE_DOCUMENT;
					break;

				default:
					printf ("state %d: defaulted\n", info->state);
					return JSON_MALFORMED_DOCUMENT;
					break;
				}
			}
			break;

		case 6:	/* label, pos name separator */
			{
				unsigned int value;	/* to avoid redundant code */
				/* perform tree sanity checks */
				assert (info->cursor != NULL);
				assert (info->cursor->type == JSON_STRING);

				switch (value = lexer (buffer, &info->p, &info->lex_state, (rcstring **) & info->lex_text))
				{
				case LEX_STRING:
					if ((temp = json_new_value (JSON_STRING)) == NULL)
						return JSON_MEMORY;
					temp->text = rcs_unwrap ((rcstring *) info->lex_text), info->lex_text = NULL;
					if (json_insert_child (info->cursor, temp) != JSON_OK)
					{
						/*TODO specify the exact error message */
						return JSON_UNKNOWN_PROBLEM;
					}
					if (info->cursor->parent == NULL)
					{
						info->state = 99;	/* finished document. only accepts whitespaces until EOF */
					}
					else
					{
						info->cursor = info->cursor->parent;
					}
					temp = NULL;
					info->state = 3;	/* finished adding a field to an object */
					break;

				case LEX_NUMBER:
					if ((temp = json_new_value (JSON_NUMBER)) == NULL)
						return JSON_MEMORY;
					temp->text = rcs_unwrap ((rcstring *) info->lex_text), info->lex_text = NULL;
					if (json_insert_child (info->cursor, temp) != JSON_OK)
					{
						/*TODO specify the exact error message */
						return JSON_UNKNOWN_PROBLEM;
					}
					if (info->cursor->parent == NULL)
					{
						info->state = 99;	/* finished document. only accepts whitespaces until EOF */
					}
					else
					{
						info->cursor = info->cursor->parent;
					}
					temp = NULL;
					info->state = 3;	/* finished adding a field to an object */
					break;

				case LEX_TRUE:
					if ((temp = json_new_value (JSON_TRUE)) == NULL)
						return JSON_MEMORY;
					if (json_insert_child (info->cursor, temp) != JSON_OK)
					{
						/*TODO specify the exact error message */
						return JSON_UNKNOWN_PROBLEM;
					}
					if (info->cursor->parent == NULL)
					{
						info->state = 99;	/* finished document. only accepts whitespaces until EOF */
					}
					else
					{
						info->cursor = info->cursor->parent;
					}
					temp = NULL;
					info->state = 3;	/* finished adding a field to an object */
					break;

				case LEX_FALSE:
					if ((temp = json_new_value (JSON_FALSE)) == NULL)
						return JSON_MEMORY;
					if (json_insert_child (info->cursor, temp) != JSON_OK)
					{
						/*TODO specify the exact error message */
						return JSON_UNKNOWN_PROBLEM;
					}
					if (info->cursor->parent == NULL)
					{
						info->state = 99;	/* finished document. only accepts whitespaces until EOF */
					}
					else
					{
						info->cursor = info->cursor->parent;
					}
					temp = NULL;
					info->state = 3;	/* finished adding a field to an object */
					break;

				case LEX_NULL:
					if ((temp = json_new_value (JSON_NULL)) == NULL)
						return JSON_MEMORY;
					if (json_insert_child (info->cursor, temp) != JSON_OK)
					{
						/*TODO specify the exact error message */
						return JSON_UNKNOWN_PROBLEM;
					}
					if (info->cursor->parent == NULL)
					{
						info->state = 99;	/* finished document. only accepts whitespaces until EOF */
					}
					else
					{
						info->cursor = info->cursor->parent;
					}
					temp = NULL;
					info->state = 3;	/* finished adding a field to an object */
					break;

				case LEX_BEGIN_OBJECT:
					info->state = 1;
					break;

				case LEX_BEGIN_ARRAY:
					info->state = 7;
					break;

				case LEX_MORE:
					return JSON_INCOMPLETE_DOCUMENT;
					break;

				case LEX_MEMORY:
					return JSON_MEMORY;
					break;

				default:
					printf ("state %d: defaulted\n", info->state);
					return JSON_MALFORMED_DOCUMENT;
					break;
				}
			}
			break;

		case 7:	/* open array */
			{
				if (info->cursor == NULL)
				{
					if ((info->cursor = json_new_array ()) == NULL)
					{
						return JSON_MEMORY;
					}
				}
				else
				{
					/* perform tree sanity checks */
					assert ((info->cursor->type == JSON_ARRAY) || (info->cursor->type == JSON_STRING));

					if ((temp = json_new_array ()) == NULL)
					{
						return JSON_MEMORY;
					}
					if (json_insert_child (info->cursor, temp) != JSON_OK)
					{
						return JSON_UNKNOWN_PROBLEM;
					}
					info->cursor = temp;
					temp = NULL;
				}
				info->state = 8;	/* just entered an array */
			}
			break;

		case 8:	/* just entered an array */
			{
				/* perform tree sanity checks */
				assert (info->cursor != NULL);
				assert (info->cursor->type == JSON_ARRAY);

				switch (lexer (buffer, &info->p, &info->lex_state, (rcstring **) & info->lex_text))
				{
				case LEX_STRING:
					if ((temp = json_new_value (JSON_STRING)) == NULL)
						return JSON_MEMORY;
					temp->text = rcs_unwrap ((rcstring *) info->lex_text), info->lex_text = NULL;
					if (json_insert_child (info->cursor, temp) != JSON_OK)
					{
						return JSON_UNKNOWN_PROBLEM;
					}
					temp = NULL;
					info->state = 9;	/* label, pre label:value separator */
					break;

				case LEX_NUMBER:
					if ((temp = json_new_value (JSON_NUMBER)) == NULL)
						return JSON_MEMORY;
					temp->text = rcs_unwrap ((rcstring *) info->lex_text), info->lex_text = NULL;
					if (json_insert_child (info->cursor, temp) != JSON_OK)
					{
						return JSON_UNKNOWN_PROBLEM;
					}
					temp = NULL;
					info->state = 9;	/* label, pre label:value separator */
					break;

				case LEX_TRUE:
					if ((temp = json_new_value (JSON_TRUE)) == NULL)
						return JSON_MEMORY;
					if (json_insert_child (info->cursor, temp) != JSON_OK)
					{
						return JSON_UNKNOWN_PROBLEM;
					}
					info->state = 9;	/* label, pre label:value separator */
					break;

				case LEX_FALSE:
					if ((temp = json_new_value (JSON_FALSE)) == NULL)
						return JSON_MEMORY;
					if (json_insert_child (info->cursor, temp) != JSON_OK)
					{
						return JSON_UNKNOWN_PROBLEM;
					}
					info->state = 9;	/* label, pre label:value separator */
					break;

				case LEX_NULL:
					if ((temp = json_new_value (JSON_NULL)) == NULL)
						return JSON_MEMORY;
					if (json_insert_child (info->cursor, temp) != JSON_OK)
					{
						return JSON_UNKNOWN_PROBLEM;
					}
					info->state = 9;	/* label, pre label:value separator */
					break;

				case LEX_BEGIN_ARRAY:
					info->state = 7;	/* open array */
					break;

				case LEX_END_ARRAY:
					if (info->cursor->parent == NULL)
					{
						/*TODO implement this */
						info->state = 99;	/* finished document. only accept whitespaces until EOF */
					}
					else
					{
						info->cursor = info->cursor->parent;
						switch (info->cursor->type)
						{
						case JSON_STRING:
							if (info->cursor->parent == NULL)
								return JSON_BAD_TREE_STRUCTURE;
							else
							{
								info->cursor = info->cursor->parent;
								if (info->cursor->type != JSON_OBJECT)
								{
									return JSON_BAD_TREE_STRUCTURE;
								}

								info->state = 3;	/* followup to adding child to array */
							}
							break;

						case JSON_ARRAY:
							info->state = 9;	/* followup to adding child to array */
							break;

						default:
							return JSON_BAD_TREE_STRUCTURE;
						}
					}
					break;

				case LEX_BEGIN_OBJECT:
					info->state = 1;	/* open object */
					break;

				case LEX_MORE:
					return JSON_INCOMPLETE_DOCUMENT;
					break;

				default:
					printf ("state %d: defaulted\n", info->state);
					return JSON_MALFORMED_DOCUMENT;
					break;
				}
			}
			break;

		case 9:	/* followup to adding child to array */
			{
				/*TODO perform tree sanity checks */
				assert (info->cursor != NULL);
				switch (lexer (buffer, &info->p, &info->lex_state, (rcstring **) & info->lex_text))
				{
				case LEX_VALUE_SEPARATOR:
					info->state = 8;
					break;

				case LEX_END_ARRAY:
					if (info->cursor->parent == NULL)
					{
						info->state = 99;	/* finished document. only accept whitespaces until EOF */
					}
					else
					{
						info->cursor = info->cursor->parent;
						switch (info->cursor->type)
						{
						case JSON_STRING:
							if (info->cursor->parent == NULL)
							{
								info->state = 99;	/* finished document. only accept whitespaces until EOF */
							}
							else
							{
								info->cursor = info->cursor->parent;
								if (info->cursor->type != JSON_OBJECT)
								{
									return JSON_BAD_TREE_STRUCTURE;
								}
								else
								{
									info->state = 3;	/* followup to adding child to array */
								}
							}
							break;

						case JSON_ARRAY:
							info->state = 9;	/* followup to adding child to array */
							break;

						default:
							return JSON_BAD_TREE_STRUCTURE;
						}
					}
					break;

				case LEX_MORE:
					return JSON_INCOMPLETE_DOCUMENT;
					break;

				default:
					printf ("state %d: defaulted\n", info->state);
					return JSON_MALFORMED_DOCUMENT;
					break;
				}
			}
			break;

		case 99:	/* finished document. only accept whitespaces until EOF */
			{
				/* perform tree sanity check */
				assert (info->cursor->parent == NULL);
				switch (lexer (buffer, &info->p, &info->lex_state, (rcstring **) & info->lex_text))
				{
				case LEX_MORE:
					return JSON_WAITING_FOR_EOF;
					break;

				case LEX_MEMORY:
					return JSON_MEMORY;
					break;

				default:
					return JSON_MALFORMED_DOCUMENT;
					break;
				}
			}
			break;

		default:
			printf ("invalid parser state %d: defaulted\n", info->state);
			return JSON_UNKNOWN_PROBLEM;
		}
	}
	info->p = NULL;
	if (info->state == 99)
		return JSON_WAITING_FOR_EOF;
	else
		return JSON_INCOMPLETE_DOCUMENT;
}



enum json_error json_parse_document (json_t **root, char *text)
{
	enum json_error error;
	struct json_parsing_info *jpi;

	assert(root != NULL);
	assert(*root == NULL);
	assert(text != NULL);

	/* initialize the parsing structure */
	jpi = malloc(sizeof(struct json_parsing_info));
	if(jpi == NULL)
	{
		return JSON_MEMORY;
	}
	json_jpi_init (jpi);

	error = json_parse_fragment (jpi, text);
	if ( (error == JSON_WAITING_FOR_EOF) || (error == JSON_OK))
	{
		*root = jpi->cursor;
		free(jpi);
		return JSON_OK;
	}
	else
	{
		free(jpi);
		return error;
	}
}


	enum json_error
json_saxy_parse (struct json_saxy_parser_status *jsps, struct json_saxy_functions *jsf, char c)
{
	/*TODO handle a string instead of a single char */
	/* temp variables */
	rcstring *temp;

	/* make sure everything is in it's place */
	assert (jsps != NULL);
	assert (jsf != NULL);
	temp = NULL;

	/* goto where we left off */
	switch (jsps->state)
	{
	case 0:		/* general state. everything goes. */
		goto state0;
		break;
	case 1:		/* parse string */
		goto state1;
		break;
	case 2:		/* parse string: escaped character */
		goto state2;
		break;
	case 3:		/* parse string: escaped unicode 1 */
		goto state3;
		break;
	case 4:		/* parse string: escaped unicode 2 */
		goto state4;
		break;
	case 5:		/* parse string: escaped unicode 3 */
		goto state5;
		break;
	case 6:		/* parse string: escaped unicode 4 */
		goto state6;
		break;
	case 7:		/* parse true: tr */
		goto state7;
		break;
	case 8:		/* parse true: tru */
		goto state8;
		break;
	case 9:		/* parse true: true */
		goto state9;
		break;
	case 10:		/* parse false: fa */
		goto state10;
		break;
	case 11:		/* parse false: fal */
		goto state11;
		break;
	case 12:		/* parse false: fals */
		goto state12;
		break;
	case 13:		/* parse false: false */
		goto state13;
		break;
	case 14:		/* parse null: nu */
		goto state14;
		break;
	case 15:		/* parse null: nul */
		goto state15;
		break;
	case 16:		/* parse null: null */
		goto state16;
		break;
	case 17:		/* parse number: 0 */
		goto state17;
		break;
	case 18:		/* parse number: start fraccional part */
		goto state18;
		break;
	case 19:		/* parse number: fraccional part */
		goto state19;
		break;
	case 20:		/* parse number: start exponent part */
		goto state20;
		break;
	case 21:		/* parse number: exponent part */
		goto state21;
		break;
	case 22:		/* parse number: exponent sign part */
		goto state22;
		break;
	case 23:		/* parse number: start negative */
		goto state23;
		break;
	case 24:		/* parse number: decimal part */
		goto state24;
		break;
	case 25:		/* open object */
		goto state25;
		break;
	case 26:		/* close object/array */
		goto state26;
		break;
	case 27:		/* sibling followup */
		goto state27;
		break;

	default:		/* oops... this should never be reached */
		return JSON_UNKNOWN_PROBLEM;
	}

      state0:			/* starting point */
	{
		switch (c)
		{
		case '\x20':
		case '\x09':
		case '\x0A':
		case '\x0D':	/* JSON insignificant white spaces */
			break;

		case '\"':	/* starting a string */
			jsps->string_length_limit_reached = 0;
			jsps->state = 1;
			break;

		case '{':
			if (jsf->open_object != NULL)
				jsf->open_object ();
			jsps->state = 25;	/*open object */
			break;

		case '}':
			if (jsf->close_object != NULL)
				jsf->close_object ();
			jsps->state = 26;	/* close object/array */
			break;

		case '[':
			if (jsf->open_array != NULL)
				jsf->open_array ();
/*                      jsps->state = 0;        // redundant*/
			break;

		case ']':
			if (jsf->close_array != NULL)
				jsf->close_array ();
			jsps->state = 26;	/* close object/array */
			break;

		case 't':
			jsps->state = 7;	/* parse true: tr */
			break;

		case 'f':
			jsps->state = 10;	/* parse false: fa */
			break;

		case 'n':
			jsps->state = 14;	/* parse null: nu */
			break;

		case ':':
			if (jsf->label_value_separator != NULL)
				jsf->label_value_separator ();
/*                      jsps->state = 0;        // redundant*/
			break;

		case ',':
			if (jsf->sibling_separator != NULL)
				jsf->sibling_separator ();
			jsps->state = 27;	/* sibling followup */
			break;

		case '0':
			jsps->string_length_limit_reached = 0;
			jsps->state = 17;	/* parse number: 0 */
			if ((jsps->temp = rcs_create (5)) == NULL)
			{
				return JSON_MEMORY;
			}
			if (rcs_catc (((rcstring *) jsps->temp), '0') != RS_OK)
			{
				return JSON_MEMORY;
			}
			break;

		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			jsps->string_length_limit_reached = 0;
			jsps->state = 24;	/* parse number: decimal */
			if ((jsps->temp = rcs_create (5)) == NULL)
			{
				return JSON_MEMORY;
			}
			if (rcs_catc (((rcstring *) jsps->temp), c) != RS_OK)
			{
				return JSON_MEMORY;
			}
			break;

		case '-':
			jsps->string_length_limit_reached = 0;
			jsps->state = 23;	/* number: */
			jsps->temp = NULL;
			if ((jsps->temp = rcs_create (5)) == NULL)
			{
				return JSON_MEMORY;
			}
			if (rcs_catc (((rcstring *) jsps->temp), '-') != RS_OK)
			{
				return JSON_MEMORY;
			}

			break;

		default:
			return JSON_ILLEGAL_CHARACTER;
			break;
		}
		return JSON_OK;
	}

      state1:			/* parse string */
	{
		switch (c)
		{
		case '\\':
			if (!jsps->string_length_limit_reached)
			{
				if (rcs_length (((rcstring *) jsps->temp)) < JSON_MAX_STRING_LENGTH - 1)	/* check if there is space for a two character escape sequence */
				{
					if (rcs_catc (((rcstring *) jsps->temp), '\\') != RS_OK)
					{
						return JSON_MEMORY;
					}
				}
				else
				{
					jsps->string_length_limit_reached = 1;
				}
			}
			jsps->state = 2;	/* parse string: escaped character */
			break;

		case '\"':	/* end of string */
			if (((rcstring *) jsps->temp) != NULL)
			{
				jsps->state = 0;	/* starting point */
				if (jsf->new_string != NULL)
					jsf->new_string (((rcstring *) ((rcstring *) jsps->temp))->text);	/*copied or integral? */
				rcs_free ((rcstring **) & jsps->temp);
			}
			else
				return JSON_UNKNOWN_PROBLEM;
			break;

		default:
			if (!jsps->string_length_limit_reached)
			{
				if (rcs_length (((rcstring *) jsps->temp)) < JSON_MAX_STRING_LENGTH)	/* check if there is space for a two character escape sequence */
				{
					if (rcs_catc (((rcstring *) jsps->temp), c) != RS_OK)
					{
						return JSON_MEMORY;
					}
				}
				else
				{
					jsps->string_length_limit_reached = 1;
				}
			}
			break;
		}
		return JSON_OK;
	}

      state2:			/* parse string: escaped character */
	{
		switch (c)
		{
		case '\"':
		case '\\':
		case '/':
		case 'b':
		case 'f':
		case 'n':
		case 'r':
		case 't':
			if (!jsps->string_length_limit_reached)
			{
				if (rcs_length (((rcstring *) jsps->temp)) < JSON_MAX_STRING_LENGTH)
				{
					if (rcs_catc (((rcstring *) jsps->temp), c) != RS_OK)
					{
						return JSON_MEMORY;
					}
				}
				else
				{
					jsps->string_length_limit_reached = 1;
				}
			}
			break;

		case 'u':
			if (!jsps->string_length_limit_reached)
			{
				if (rcs_length (((rcstring *) jsps->temp)) < JSON_MAX_STRING_LENGTH - 4)
				{
					if (rcs_catc (((rcstring *) jsps->temp), 'u') != RS_OK)
					{
						return JSON_MEMORY;
					}
				}
				else
				{
					jsps->string_length_limit_reached = 1;
				}
			}
			jsps->state = 3;	/* parse string: escaped unicode 1; */
			break;

		default:
			return JSON_ILLEGAL_CHARACTER;
			break;
		}
		return JSON_OK;
	}

      state3:			/* parse string: escaped unicode 1 */
	{
		switch (c)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			if (!jsps->string_length_limit_reached)
			{
				if (rcs_length (((rcstring *) jsps->temp)) < JSON_MAX_STRING_LENGTH - 3)
				{
					if (rcs_catc (((rcstring *) jsps->temp), 'u') != RS_OK)
					{
						return JSON_MEMORY;
					}
				}
				else
				{
					jsps->string_length_limit_reached = 1;
				}
			}
			jsps->state = 4;	/* parse string. escaped unicode 2 */
			break;

		default:
			return JSON_ILLEGAL_CHARACTER;
		}
		return JSON_OK;
	}

      state4:			/* parse string: escaped unicode 2 */
	{
		switch (c)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			if (!jsps->string_length_limit_reached)
			{
				if (rcs_length (((rcstring *) jsps->temp)) < JSON_MAX_STRING_LENGTH - 2)
				{
					if (rcs_catc (((rcstring *) jsps->temp), c) != RS_OK)
					{
						return JSON_MEMORY;
					}
				}
				else
				{
					jsps->string_length_limit_reached = 1;
				}
			}
			jsps->state = 5;	/* parse string. escaped unicode 3 */
			break;

		default:
			return JSON_ILLEGAL_CHARACTER;
		}
		return JSON_OK;
	}

      state5:			/* parse string: escaped unicode 3 */
	{
		switch (c)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			if (!jsps->string_length_limit_reached)
			{
				if (rcs_length (((rcstring *) jsps->temp)) < JSON_MAX_STRING_LENGTH - 1)
				{
					if (rcs_catc (((rcstring *) jsps->temp), c) != RS_OK)
					{
						return JSON_MEMORY;
					}
				}
				else
				{
					jsps->string_length_limit_reached = 1;
				}
			}
			jsps->state = 6;	/* parse string. escaped unicode 4 */
			break;

		default:
			return JSON_ILLEGAL_CHARACTER;
		}
		return JSON_OK;
	}

      state6:			/* parse string: escaped unicode 4 */
	{
		switch (c)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			if (!jsps->string_length_limit_reached)
			{
				if (rcs_length (((rcstring *) jsps->temp)) < JSON_MAX_STRING_LENGTH)
				{
					if (rcs_catc (((rcstring *) jsps->temp), c) != RS_OK)
					{
						return JSON_MEMORY;
					}
				}
				else
				{
					jsps->string_length_limit_reached = 1;
				}
			}
			jsps->state = 1;	/* parse string */
			break;

		default:
			return JSON_ILLEGAL_CHARACTER;
		}
		return JSON_OK;
	}

      state7:			/* parse true: tr */
	{
		if (c != 'r')
		{
			return JSON_ILLEGAL_CHARACTER;
		}

		jsps->state = 8;	/* parse true: tru */
		return JSON_OK;
	}

      state8:			/* parse true: tru */
	{
		if (c != 'u')
		{
			return JSON_ILLEGAL_CHARACTER;
		}

		jsps->state = 9;	/* parse true: true */
		return JSON_OK;
	}

      state9:			/* parse true: true */
	{
		if (c != 'e')
		{
			return JSON_ILLEGAL_CHARACTER;
		}

		jsps->state = 0;	/* back to general state. */
		if (jsf->new_true != NULL)
			jsf->new_true ();
		return JSON_OK;
	}

      state10:			/* parse false: fa */
	{
		if (c != 'a')
		{
			return JSON_ILLEGAL_CHARACTER;
		}

		jsps->state = 11;	/* parse true: fal */
		return JSON_OK;
	}

      state11:			/* parse false: fal */
	{
		if (c != 'l')
		{
			return JSON_ILLEGAL_CHARACTER;
		}

		jsps->state = 12;	/* parse true: fals */
		return JSON_OK;
	}

      state12:			/* parse false: fals */
	{
		if (c != 's')
		{
			return JSON_ILLEGAL_CHARACTER;
		}

		jsps->state = 13;	/* parse true: false */
		return JSON_OK;
	}

      state13:			/* parse false: false */
	{
		if (c != 'e')
		{
			return JSON_ILLEGAL_CHARACTER;
		}

		jsps->state = 0;	/* general state. everything goes. */
		if (jsf->new_false != NULL)
			jsf->new_false ();
		return JSON_OK;
	}

      state14:			/* parse null: nu */
	{
		if (c != 'u')
		{
			return JSON_ILLEGAL_CHARACTER;
		}

		jsps->state = 15;	/* parse null: nul */
		return JSON_OK;
	}

      state15:			/* parse null: nul */
	{
		if (c != 'l')
		{
			return JSON_ILLEGAL_CHARACTER;
		}

		jsps->state = 16;	/* parse null: null */
		return JSON_OK;
	}

      state16:			/* parse null: null */
	{
		if (c != 'l')
		{
			return JSON_ILLEGAL_CHARACTER;
		}

		jsps->state = 0;	/* general state. everything goes. */
		if (jsf->new_null != NULL)
			jsf->new_null ();
		return JSON_OK;
	}

      state17:			/* parse number: 0 */
	{
		switch (c)
		{
		case '.':
			if ((jsps->temp = rcs_create (5)) == NULL)
			{
				return JSON_MEMORY;
			}
			if (rcs_catc (((rcstring *) jsps->temp), '.') != RS_OK)
			{
				return JSON_MEMORY;
			}
			jsps->state = 18;	/* parse number: fraccional part */
			break;

		case '\x20':
		case '\x09':
		case '\x0A':
		case '\x0D':	/* JSON insignificant white spaces */
			if (((rcstring *) jsps->temp) == NULL)
				return JSON_MEMORY;
			if (jsf->new_number != NULL)
			{
				jsf->new_number (((rcstring *) jsps->temp)->text);
			}
			rcs_free ((rcstring **) & jsps->temp);

			jsps->state = 0;
			break;

		case '}':
			if (((rcstring *) jsps->temp) == NULL)
				return JSON_MEMORY;
			if (jsf->new_number != NULL)
			{
				jsf->new_number (((rcstring *) jsps->temp)->text);
			}
			rcs_free ((rcstring **) & jsps->temp);

			if (jsf->open_object != NULL)
				jsf->close_object ();
			jsps->state = 26;	/* close object/array */
			break;

		case ']':

			if (((rcstring *) jsps->temp) == NULL)
				return JSON_MEMORY;
			if (jsf->new_number != NULL)
			{
				jsf->new_number (((rcstring *) jsps->temp)->text);
			}
			rcs_free ((rcstring **) & jsps->temp);

			if (jsf->open_object != NULL)
				jsf->close_array ();
			jsps->state = 26;	/* close object/array */
			break;

		case ',':

			if (((rcstring *) jsps->temp) == NULL)
				return JSON_MEMORY;
			if (jsf->new_number != NULL)
			{
				jsf->new_number (((rcstring *) jsps->temp)->text);
			}
			rcs_free ((rcstring **) & jsps->temp);

			if (jsf->open_object != NULL)
				jsf->label_value_separator ();
			jsps->state = 27;	/* sibling followup */
			break;

		default:
			return JSON_ILLEGAL_CHARACTER;
			break;
		}

		return JSON_OK;
	}

      state18:			/* parse number: start fraccional part */
	{
		switch (c)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (!jsps->string_length_limit_reached)
			{
				if (rcs_length (((rcstring *) jsps->temp)) < JSON_MAX_STRING_LENGTH / 2)
				{
					if (rcs_catc (((rcstring *) jsps->temp), c) != RS_OK)
					{
						return JSON_MEMORY;
					}
				}
				else
				{
					jsps->string_length_limit_reached = 1;
				}
			}
			jsps->state = 19;	/* parse number: fractional part */
			break;

		default:
			return JSON_ILLEGAL_CHARACTER;
			break;
		}
		return JSON_OK;
	}

      state19:			/* parse number: fraccional part */
	{
		switch (c)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (!jsps->string_length_limit_reached)
			{
				if (rcs_length (((rcstring *) jsps->temp)) < JSON_MAX_STRING_LENGTH / 2)
				{
					if (rcs_catc (((rcstring *) jsps->temp), c) != RS_OK)
					{
						return JSON_MEMORY;
					}
				}
				else
				{
					jsps->string_length_limit_reached = 1;
				}
			}
/*                      jsps->state = 19;       // parse number: fractional part*/
			break;

		case 'e':
		case 'E':
			if (rcs_catc (((rcstring *) jsps->temp), c) != RS_OK)
			{
				return JSON_MEMORY;
			}

			jsps->state = 20;	/* parse number: start exponent part */
			break;


		case '\x20':
		case '\x09':
		case '\x0A':
		case '\x0D':	/* JSON insignificant white spaces */

			if (((rcstring *) jsps->temp) == NULL)
				return JSON_MEMORY;
			if (jsf->new_number != NULL)
			{
				jsf->new_number (((rcstring *) jsps->temp)->text);
			}
			rcs_free ((rcstring **) & jsps->temp);

			jsps->state = 0;
			break;

		case '}':

			if (((rcstring *) jsps->temp) == NULL)
				return JSON_MEMORY;
			if (jsf->new_number != NULL)
			{
				jsf->new_number (((rcstring *) jsps->temp)->text);
			}
			rcs_free ((rcstring **) & jsps->temp);

			if (jsf->open_object != NULL)
				jsf->close_object ();
			jsps->state = 26;	/* close object/array */
			break;

		case ']':
			if (jsf->new_number != NULL)
			{
				if (((rcstring *) jsps->temp) == NULL)
					return JSON_MEMORY;
				jsf->new_number (((rcstring *) jsps->temp)->text);
				rcs_free ((rcstring **) & jsps->temp);
			}
			else
			{
				rcs_free ((rcstring **) & jsps->temp);
				jsps->temp = NULL;
			}
			if (jsf->open_object != NULL)
				jsf->close_array ();
			jsps->state = 26;	/* close object/array */
			break;

		case ',':

			if (((rcstring *) jsps->temp) == NULL)
				return JSON_MEMORY;
			if (jsf->new_number != NULL)
			{
				jsf->new_number (((rcstring *) jsps->temp)->text);
			}
			rcs_free ((rcstring **) & jsps->temp);

			if (jsf->label_value_separator != NULL)
				jsf->label_value_separator ();
			jsps->state = 27;	/* sibling followup */
			break;


		default:
			return JSON_ILLEGAL_CHARACTER;
			break;
		}
		return JSON_OK;
	}

      state20:			/* parse number: start exponent part */
	{
		switch (c)
		{
		case '+':
		case '-':
			jsps->string_length_limit_reached = 0;
			if (rcs_catc (((rcstring *) jsps->temp), c) != RS_OK)
			{
				return JSON_MEMORY;
			}

			jsps->state = 22;	/* parse number: exponent sign part */
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (!jsps->string_length_limit_reached)
			{
				if (rcs_length (((rcstring *) jsps->temp)) < JSON_MAX_STRING_LENGTH)
				{
					if (rcs_catc (((rcstring *) jsps->temp), c) != RS_OK)
					{
						return JSON_MEMORY;
					}

				}
				else
				{
					jsps->string_length_limit_reached = 1;
				}
			}
			jsps->state = 21;	/* parse number: exponent part */
			break;

		default:
			return JSON_ILLEGAL_CHARACTER;
			break;
		}
		return JSON_OK;
	}

      state21:			/* parse number: exponent part */
	{
		switch (c)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (!jsps->string_length_limit_reached)
			{
				if (rcs_length (((rcstring *) jsps->temp)) < JSON_MAX_STRING_LENGTH)
				{
					if (rcs_catc (((rcstring *) jsps->temp), c) != RS_OK)
					{
						return JSON_MEMORY;
					}
				}
				else
				{
					jsps->string_length_limit_reached = 1;
				}
			}
/*                              jsps->state = 21;       // parse number: exponent part*/
			break;

		case '\x20':
		case '\x09':
		case '\x0A':
		case '\x0D':	/* JSON insignificant white spaces */

			if (((rcstring *) jsps->temp) == NULL)
				return JSON_MEMORY;
			if (jsf->new_number != NULL)
			{
				jsf->new_number (((rcstring *) jsps->temp)->text);
			}
			rcs_free ((rcstring **) & jsps->temp);

			jsps->state = 0;
			break;

		case '}':
			if (((rcstring *) jsps->temp) == NULL)
				return JSON_MEMORY;
			if (jsf->new_number != NULL)
			{
				jsf->new_number (((rcstring *) jsps->temp)->text);
			}
			rcs_free ((rcstring **) & jsps->temp);

			if (jsf->open_object != NULL)
				jsf->close_object ();
			jsps->state = 26;	/* close object */
			break;

		case ']':
			if (jsf->new_number != NULL)
			{
				if (((rcstring *) jsps->temp) == NULL)
					return JSON_MEMORY;
				jsf->new_number (((rcstring *) jsps->temp)->text);
				free ((rcstring *) jsps->temp);
				jsps->temp = NULL;
			}
			else
			{
				free (((rcstring *) jsps->temp));
				jsps->temp = NULL;
			}
			if (jsf->open_object != NULL)
				jsf->close_array ();
			jsps->state = 26;	/* close object/array */
			break;

		case ',':
			if (jsf->new_number != NULL)
			{
				if (((rcstring *) jsps->temp) == NULL)
					return JSON_MEMORY;
				jsf->new_number (((rcstring *) jsps->temp)->text);
				free (((rcstring *) jsps->temp));
				jsps->temp = NULL;
			}
			else
			{
				free ((rcstring *) jsps->temp);
				jsps->temp = NULL;
			}
			if (jsf->label_value_separator != NULL)
				jsf->label_value_separator ();
			jsps->state = 27;	/* sibling followup */
			break;

		default:
			return JSON_ILLEGAL_CHARACTER;
			break;
		}
		return JSON_OK;
	}

      state22:			/* parse number: start exponent part */
	{
		switch (c)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (!jsps->string_length_limit_reached)
			{
				if (rcs_length (((rcstring *) jsps->temp)) < JSON_MAX_STRING_LENGTH)
				{
					rcs_catc (((rcstring *) jsps->temp), c);
				}
				else
				{
					jsps->string_length_limit_reached = 1;
				}
			}
			jsps->state = 21;	/* parse number: exponent part */
			break;

		default:
			return JSON_ILLEGAL_CHARACTER;
			break;
		}
		return JSON_OK;
	}

      state23:			/* parse number: start negative */
	{
		switch (c)
		{
		case '0':
			rcs_catc (((rcstring *) jsps->temp), c);
			jsps->state = 17;	/* parse number: 0 */
			break;

		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (!jsps->string_length_limit_reached)
			{
				if (rcs_length (((rcstring *) jsps->temp)) < JSON_MAX_STRING_LENGTH / 2)
				{
					if ((jsps->temp = rcs_create (5)) == NULL)
					{
						return JSON_MEMORY;
					}
					if (rcs_catc (((rcstring *) jsps->temp), c) != RS_OK)
					{
						return JSON_MEMORY;
					}
					else
					{
						jsps->string_length_limit_reached = 1;
					}
				}
			}
			jsps->state = 24;	/* parse number: start decimal part */
			break;

		default:
			return JSON_ILLEGAL_CHARACTER;
			break;
		}
		return JSON_OK;
	}

      state24:			/* parse number: decimal part */
	{
		switch (c)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (!jsps->string_length_limit_reached)
			{
				if (rcs_length (((rcstring *) jsps->temp)) < JSON_MAX_STRING_LENGTH / 2)
				{
					if ((jsps->temp = rcs_create (5)) == NULL)
					{
						return JSON_MEMORY;
					}
					if (rcs_catc (((rcstring *) jsps->temp), c) != RS_OK)
					{
						return JSON_MEMORY;
					}
				}
				else
				{
					jsps->string_length_limit_reached = 1;
				}
			}
/*                              jsps->state = 24;       // parse number: decimal part*/
			break;

		case '.':
			if ((jsps->temp = rcs_create (5)) == NULL)
			{
				return JSON_MEMORY;
			}
			if (rcs_catc (((rcstring *) jsps->temp), '.') != RS_OK)
			{
				return JSON_MEMORY;
			}

			jsps->state = 18;	/* parse number: start exponent part */
			break;

		case 'e':
		case 'E':
			if ((jsps->temp = rcs_create (5)) == NULL)
			{
				return JSON_MEMORY;
			}
			if (rcs_catc (((rcstring *) jsps->temp), c) != RS_OK)
			{
				return JSON_MEMORY;
			}

			jsps->string_length_limit_reached = 0;	/* reset to accept the exponential part */
			jsps->state = 20;	/* parse number: start exponent part */
			break;

		case '\x20':
		case '\x09':
		case '\x0A':
		case '\x0D':	/* JSON insignificant white spaces */
			if (((rcstring *) jsps->temp) == NULL)
				return JSON_MEMORY;
			if (jsf->new_number != NULL)
			{
				jsf->new_number (((rcstring *) jsps->temp)->text);
			}
			rcs_free ((rcstring **) & jsps->temp);

			jsps->state = 0;
			break;

		case '}':
			if (((rcstring *) jsps->temp) == NULL)
				return JSON_MEMORY;
			if (jsf->new_number != NULL)
			{
				jsf->new_number (((rcstring *) jsps->temp)->text);
			}
			rcs_free ((rcstring **) & jsps->temp);

			if (jsf->open_object != NULL)
				jsf->close_object ();
			jsps->state = 26;	/* close object/array */
			break;

		case ']':
			if (((rcstring *) jsps->temp) == NULL)
				return JSON_MEMORY;
			if (jsf->new_number != NULL)
			{
				jsf->new_number (((rcstring *) jsps->temp)->text);
			}
			rcs_free ((rcstring **) & jsps->temp);

			if (jsf->open_object != NULL)
				jsf->close_array ();
			jsps->state = 26;	/* close object/array */
			break;

		case ',':
			if (((rcstring *) jsps->temp) == NULL)
				return JSON_MEMORY;
			if (jsf->new_number != NULL)
			{
				jsf->new_number (((rcstring *) jsps->temp)->text);
			}
			rcs_free ((rcstring **) & jsps->temp);

			if (jsf->label_value_separator != NULL)
				jsf->label_value_separator ();
			jsps->state = 27;	/* sibling followup */
			break;

		default:
			return JSON_ILLEGAL_CHARACTER;
			break;
		}
		return JSON_OK;
	}

      state25:			/* open object */
	{
		switch (c)
		{
		case '\x20':
		case '\x09':
		case '\x0A':
		case '\x0D':	/* JSON insignificant white spaces */
			break;

		case '\"':
			jsps->temp = NULL;
			jsps->state = 1;
			break;

		case '}':
			if (jsf->close_object != NULL)
				jsf->close_object ();
			jsps->state = 26;	/* close object */
			break;

		default:
			return JSON_ILLEGAL_CHARACTER;
			break;
		}
		return JSON_OK;
	}

      state26:			/* close object/array */
	{
		switch (c)
		{
		case '\x20':
		case '\x09':
		case '\x0A':
		case '\x0D':	/* JSON insignificant white spaces */
			break;

		case '}':
			if (jsf->close_object != NULL)
				jsf->close_object ();
/*                      jsp->state = 26;        // close object*/
			break;

		case ']':
			if (jsf->close_array != NULL)
				jsf->close_array ();
/*                      jsps->state = 26;       // close object/array*/
			break;

		case ',':
			if (jsf->sibling_separator != NULL)
				jsf->sibling_separator ();
			jsps->state = 27;	/* sibling followup */
			break;

		default:
			return JSON_ILLEGAL_CHARACTER;
			break;
		}
		return JSON_OK;
	}

      state27:			/* sibling followup */
	{
		switch (c)
		{
		case '\x20':
		case '\x09':
		case '\x0A':
		case '\x0D':	/* JSON insignificant white spaces */
			break;

		case '\"':
			jsps->state = 1;
			jsps->temp = NULL;
			break;

		case '{':
			if (jsf->open_object != NULL)
				jsf->open_object ();
			jsps->state = 25;	/*open object */
			break;

		case '[':
			if (jsf->open_array != NULL)
				jsf->open_array ();
/*                      jsps->state = 0;        // redundant*/
			break;

		case 't':
			jsps->state = 7;	/* parse true: tr */
			break;

		case 'f':
			jsps->state = 10;	/* parse false: fa */
			break;

		case 'n':
			jsps->state = 14;	/* parse null: nu */
			break;

		case '0':
			jsps->state = 17;	/* parse number: 0 */
			if ((jsps->temp = rcs_create (5)) == NULL)
			{
				return JSON_MEMORY;
			}
			if (rcs_catc (((rcstring *) jsps->temp), '0') != RS_OK)
			{
				return JSON_MEMORY;
			}
			break;

		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			jsps->state = 24;	/* parse number: decimal */
			if ((jsps->temp = rcs_create (5)) == NULL)
			{
				return JSON_MEMORY;
			}
			if (rcs_catc (((rcstring *) jsps->temp), c) != RS_OK)
			{
				return JSON_MEMORY;
			}
			break;

		case '-':
			jsps->state = 23;	/* number: */
			if ((jsps->temp = rcs_create (5)) == NULL)
			{
				return JSON_MEMORY;
			}
			if (rcs_catc (((rcstring *) jsps->temp), '-') != RS_OK)
			{
				return JSON_MEMORY;
			}
			break;

		default:
			return JSON_ILLEGAL_CHARACTER;
			break;
		}
		return JSON_OK;
	}

	return JSON_UNKNOWN_PROBLEM;
}


json_t *
json_find_first_label (const json_t * object, const char *text_label)
{
	json_t *cursor;

	assert (object != NULL);
	assert (text_label != NULL);
	assert (object->type == JSON_OBJECT);

	if (object->child == NULL)
		return NULL;
	cursor = object->child;
	while (cursor != NULL)
	{
		if (strncmp (cursor->text, text_label, strlen (text_label)) == 0)
			return cursor;
		cursor = cursor->next;
	}
	return NULL;
}
