import { NextResponse } from 'next/server'
import { supabase } from '../../../../lib/supabase'

export async function POST(request: Request) {
  try {
    const body = await request.json()
    const { status, battery } = body

    if (!status) {
      return NextResponse.json({ error: 'Missing status string' }, { status: 400 })
    }

    const { data, error } = await supabase
      .from('dog_status')
      .upsert([
        {
          id: 1,
          status,
          battery: battery ?? 100,
          updated_at: new Date().toISOString()
        }
      ])
      .select()

    if (error) {
      return NextResponse.json({ error: error.message }, { status: 500 })
    }

    return NextResponse.json({ success: true, dog_status: data }, { status: 200 })
  } catch (err: any) {
    return NextResponse.json({ error: err.message || 'Internal Server Error' }, { status: 500 })
  }
}