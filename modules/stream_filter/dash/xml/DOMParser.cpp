/*
 * DOMParser.cpp
 *****************************************************************************
 * Copyright (C) 2010 - 2011 Klagenfurt University
 *
 * Created on: Aug 10, 2010
 * Authors: Christopher Mueller <christopher.mueller@itec.uni-klu.ac.at>
 *          Christian Timmerer  <christian.timmerer@itec.uni-klu.ac.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "DOMParser.h"

using namespace dash::xml;
using namespace dash::http;
using namespace dash::mpd;

DOMParser::DOMParser    (stream_t *stream)
{
    this->stream = stream;
    this->init();
}
DOMParser::~DOMParser   ()
{
    if(this->vlc_reader)
        xml_ReaderDelete(this->vlc_reader);
}

Node*   DOMParser::getRootNode              ()
{
    return this->root;
}
bool    DOMParser::parse                    ()
{
    this->vlc_xml = xml_Create(this->stream);

    if(!this->vlc_xml)
        return false;

    this->vlc_reader = xml_ReaderCreate(this->vlc_xml, this->stream);

    if(!this->vlc_reader)
        return false;

    this->root = this->processNode();

    return true;
}
Node*   DOMParser::processNode              ()
{
    const char *data;
    int type = xml_ReaderNextNode(this->vlc_reader, &data);
    if(type != XML_READER_TEXT && type != XML_READER_NONE && type != XML_READER_ENDELEM)
    {
        Node *node = new Node();

        std::string name    = data;
        bool        isEmpty = xml_ReaderIsEmptyElement(this->vlc_reader);
        node->setName(name);

        this->addAttributesToNode(node);

        if(isEmpty)
            return node;

        Node *subnode = NULL;

        while(subnode = this->processNode())
            node->addSubNode(subnode);

        return node;
    }
    return NULL;
}
void    DOMParser::addAttributesToNode      (Node *node)
{
    const char *attrValue;
    const char *attrName;

    while((attrName = xml_ReaderNextAttr(this->vlc_reader, &attrValue)) != NULL)
    {
        std::string key     = attrName;
        std::string value   = attrValue;
        node->addAttribute(key, value);
    }
}
void    DOMParser::print                    (Node *node, int offset)
{
    for(int i = 0; i < offset; i++)
        msg_Dbg(this->stream, " ");

    msg_Dbg(this->stream, "%s", node->getName().c_str());

    std::vector<std::string> keys = node->getAttributeKeys();

    for(size_t i = 0; i < keys.size(); i++)
        msg_Dbg(this->stream, " %s=%s", keys.at(i).c_str(), node->getAttributeValue(keys.at(i)).c_str());

    msg_Dbg(this->stream, "\n");

    offset++;

    for(size_t i = 0; i < node->getSubNodes().size(); i++)
    {
        this->print(node->getSubNodes().at(i), offset);
    }
}
void    DOMParser::init                     ()
{
    this->root          = NULL;
    this->vlc_reader    = NULL;
}
void    DOMParser::print                    ()
{
    this->print(this->root, 0);
}
Profile DOMParser::getProfile               (dash::xml::Node *node)
{
    std::string profile = node->getAttributeValue("profiles");

    if(!profile.compare("urn:mpeg:mpegB:profile:dash:isoff-basic-on-demand:cm"))
        return dash::mpd::BasicCM;

    return dash::mpd::NotValid;
}
bool    DOMParser::isDash                   ()
{
    const uint8_t *peek, *peek_end;

    int64_t i_size = stream_Peek(this->stream, &peek, 2048);
    if(i_size < 1)
        return false;

    peek_end = peek + i_size;
    while(peek <= peek_end)
    {
        const char *p = strstr((const char*)peek, "urn:mpeg:mpegB:schema:DASH:MPD:DIS2011");
        if (p != NULL)
            return true;
        peek++;
    };

    return false;
}